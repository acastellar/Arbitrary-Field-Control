#version 450

layout(location = 0) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

const vec2 center = vec2(0.5f, 0.5f);
void main() {

    float dist = length(gl_PointCoord.xy - center);
    if (dist > 0.5f) {
        discard;
    }

    outColor = vec4(fragColor.rgb, 0.9f);
}

