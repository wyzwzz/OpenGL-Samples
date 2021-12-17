#include "denoiser.h"

Denoiser::Denoiser() : m_useTemportal(false) {}

void Denoiser::Reprojection(const FrameInfo &frameInfo) {
    int height = m_accColor.m_height;
    int width = m_accColor.m_width;
    Matrix4x4 preWorldToScreen =
        m_preFrameInfo.m_matrix[m_preFrameInfo.m_matrix.size() - 1];
    Matrix4x4 preWorldToCamera =
        m_preFrameInfo.m_matrix[m_preFrameInfo.m_matrix.size() - 2];
#pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // TODO: Reproject
            m_valid(x, y) = false;
            m_misc(x, y) = Float3(0.f);

            int cur_id = frameInfo.m_id(x,y);
            if(cur_id == -1){
                continue;
            }
            Matrix4x4 world_to_local = Inverse(frameInfo.m_matrix[cur_id]);
            const auto& pre_local_to_world = m_preFrameInfo.m_matrix[cur_id];
            const auto& cur_pos = frameInfo.m_position(x,y);
            auto screen_pos = preWorldToScreen(pre_local_to_world(world_to_local(cur_pos,Float3::EType::Point),
                                                                  Float3::EType::Point),
                                               Float3::EType::Point);
            if(screen_pos.x< 0.0 || screen_pos.x >= width ||
                screen_pos.y< 0.0 || screen_pos.y >= height){
                continue;
            }
            else{
                int pre_x = screen_pos.x;
                int pre_y = screen_pos.y;
                int pre_id = m_preFrameInfo.m_id(pre_x,pre_y);
                if(cur_id == pre_id){
                    m_valid(x,y) = true;
                    m_misc(x,y) = m_accColor(pre_x,pre_y);
                }
            }
        }
    }
    std::swap(m_misc, m_accColor);
}

void Denoiser::TemporalAccumulation(const Buffer2D<Float3> &curFilteredColor) {
    int height = m_accColor.m_height;
    int width = m_accColor.m_width;
    int kernelRadius = 3;
#pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // TODO: Temporal clamp
            Float3 color = m_accColor(x, y);
            // TODO: Exponential moving average
            int x_start = std::max(0,x-kernelRadius);
            int x_end = std::min(width-1,x+kernelRadius);
            int y_start = std::max(0,y-kernelRadius);
            int y_end = std::min(height-1,y+kernelRadius);
            int count = 0;
            Float3 mean,sigma;
            for(int i=x_start;i<=x_end;i++){
                for(int j=y_start;j<=y_end;j++){
                    count++;
                    mean += curFilteredColor(i,j);
                }
            }
            mean /= count;
            for(int i=x_start;i<=x_end;i++){
                for(int j=y_start;j<=y_end;j++){
                    sigma += Sqr(curFilteredColor(i,j)-mean);
                }
            }
            sigma /= count;

            color = Clamp(color,mean-sigma*m_colorBoxK,mean+sigma*m_colorBoxK);

            float alpha = 1.0f;
            if(m_valid(x,y)){
                alpha = m_alpha;
            }
            m_misc(x, y) = Lerp(color, curFilteredColor(x, y), alpha);
        }
    }
    std::swap(m_misc, m_accColor);
}

Buffer2D<Float3> Denoiser::Filter(const FrameInfo &frameInfo) {
    int height = frameInfo.m_beauty.m_height;
    int width = frameInfo.m_beauty.m_width;
    Buffer2D<Float3> filteredImage = CreateBuffer2D<Float3>(width, height);
    int kernelRadius = 16;
#pragma omp parallel for collapse(2)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // TODO: Joint bilateral filter
            int x_start = std::max(0,x-kernelRadius);
            int x_end = std::min(width-1,x+kernelRadius);
            int y_start = std::max(0,y-kernelRadius);
            int y_end = std::min(height-1,y+kernelRadius);
            Float3 finalColor;
            float weight_sum = 0.f;
            const auto& center_pos = frameInfo.m_position(x,y);
            const auto& center_normal = frameInfo.m_normal(x,y);
            const auto& center_color = frameInfo.m_beauty(x,y);
            for(int i = x_start; i <= x_end;i++){
                for(int j = y_start;j <= y_end;j++){
                    if(i==x && j ==y){
                        weight_sum += 1.f;
                        finalColor += center_color;
                        continue;
                    }

                    const auto& cur_pos = frameInfo.m_position(i,j);
                    const auto& cur_normal = frameInfo.m_normal(i,j);
                    const auto& cur_color = frameInfo.m_beauty(i,j);

                    float pos_dist = SqrDistance(cur_pos,center_pos)/(2.f*(m_sigmaCoord));
                    float color_dist = SqrDistance(cur_color,center_color)/(2.f*(m_sigmaColor));
                    float normal_dist = SafeAcos(Dot(cur_normal,center_normal))/(2.f*(m_sigmaNormal));
                    float plane_dist = 0.f;
                    if(pos_dist!=0.f){
                        plane_dist = Dot(center_normal,Normalize(cur_pos-center_pos))/(2.f*(m_sigmaPlane));
                    }
                    float weight = std::exp(-pos_dist-color_dist-normal_dist-plane_dist);
                    weight_sum += weight;
                    finalColor += cur_color * weight;
                }
            }
            filteredImage(x,y) = finalColor / weight_sum;
        }
    }
    return filteredImage;
}

void Denoiser::Init(const FrameInfo &frameInfo, const Buffer2D<Float3> &filteredColor) {
    m_accColor.Copy(filteredColor);
    int height = m_accColor.m_height;
    int width = m_accColor.m_width;
    m_misc = CreateBuffer2D<Float3>(width, height);
    m_valid = CreateBuffer2D<bool>(width, height);
}

void Denoiser::Maintain(const FrameInfo &frameInfo) { m_preFrameInfo = frameInfo; }

Buffer2D<Float3> Denoiser::ProcessFrame(const FrameInfo &frameInfo) {
    // Filter current frame
    Buffer2D<Float3> filteredColor;
    filteredColor = Filter(frameInfo);

    // Reproject previous frame color to current
    if (m_useTemportal) {
        Reprojection(frameInfo);
        TemporalAccumulation(filteredColor);
    } else {
        Init(frameInfo, filteredColor);
    }

    // Maintain
    Maintain(frameInfo);
    if (!m_useTemportal) {
        m_useTemportal = true;
    }
    return m_accColor;
}
