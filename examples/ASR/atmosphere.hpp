//
// Created by wyz on 2022/6/27.
//

#ifndef OPENGL_SAMPLES_ATMOSPHERE_HPP
#define OPENGL_SAMPLES_ATMOSPHERE_HPP
#include "demo.hpp"

struct AtmosphereProperties{
    vec3f rayleigh_scattering = {5.802f,13.558f,33.1f};//10^(-6)m^(-1)
    float rayleigh_density_h = 8.f;//km

    float mie_scattering = 3.996f;
    float mie_asymmetry_g = 0.8f;
    float mie_absorption = 4.4f;
    float mie_density_h = 1.2f;

    vec3f ozone_absorption = {0.65f,1.881f,0.085f};
    float ozone_center_h = 25;//km
    float ozone_width = 30;//km

    float ground_radius = 6360;//km
    float top_atmosphere_radius = 6460;//km
    float padding = 0;

    AtmosphereProperties toStdUnit() const{
        AtmosphereProperties ret = *this;

        ret.rayleigh_scattering *= 1e-6f;
        ret.rayleigh_density_h *= 1e3f;

        ret.mie_scattering *= 1e-6f;
        ret.mie_absorption *= 1e-6f;
        ret.mie_density_h *= 1e3f;

        ret.ozone_absorption *= 1e-6f;
        ret.ozone_center_h *= 1e3f;
        ret.ozone_width *= 1e3f;

        ret.ground_radius *= 1e3f;
        ret.top_atmosphere_radius *= 1e3f;

        return ret;
    }
};

static_assert(sizeof(AtmosphereProperties) == 64,"");

#endif // OPENGL_SAMPLES_ATMOSPHERE_HPP
