#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) out vec3 weight;

void main()
{
    gl_Position = gl_in[0].gl_Position;
    weight = vec3(1,0,0);
    EmitVertex();
    gl_Position = gl_in[1].gl_Position;
    weight = vec3(0,1,0);
    EmitVertex();
    gl_Position = gl_in[2].gl_Position;
    weight = vec3(0,0,1);
    EmitVertex();

    EndPrimitive();
}