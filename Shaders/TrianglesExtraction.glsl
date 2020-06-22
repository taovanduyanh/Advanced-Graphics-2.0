#version 430 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in Vertex {
    vec4 colour;
}   IN[];

out Vertex {
    vec4 colour;
}   OUT;

layout(binding = 1, offset = 0) uniform atomic_uint triangleCount;

void main() {
    for (int i = 0; i < gl_in.length(); ++i) {
        OUT.colour = IN[i].colour;
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
    atomicCounterIncrement(triangleCount);
}