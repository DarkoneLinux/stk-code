#include "utils/spm_layout.h"
#include "../utils/get_world_location.vert"

layout(push_constant) uniform Constants
{
    vec3 m_wind_direction;
} u_push_constants;

void main()
{
    vec3 offset = sin(u_push_constants.m_wind_direction * (v_position.y * 0.1));
    offset += vec3(cos(u_push_constants.m_wind_direction) * 0.7);

    vec4 v_world_position = getWorldPosition(
        u_object_buffer.m_objects[gl_InstanceIndex].m_position.xyz + offset *
        v_color.r,
        u_object_buffer.m_objects[gl_InstanceIndex].m_rotation,
        u_object_buffer.m_objects[gl_InstanceIndex].m_scale.xyz, v_position);
    gl_Position = u_camera.m_projection_view_matrix * v_world_position;
    f_vertex_color = vec4(1.0);
    f_uv = v_uv;
    f_uv_two = v_uv_two;
    f_material_id = u_object_buffer.m_objects[gl_InstanceIndex].m_material_id;
}