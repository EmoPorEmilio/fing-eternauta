#pragma once

enum class ShaderType {
    Color,
    Model,
    Skinned
};

struct Renderable {
    ShaderType shader = ShaderType::Model;
};
