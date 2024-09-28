#version 330 core

out vec4 out_color;

uniform vec2 size;
uniform float time;
uniform mat4 view;

const int MAX_MARCHING_STEPS = 100;
const float MAX_DIST = 1000.0;
const float MIN_DIST = 0.0;
const float EPSILON = 0.0001;

float
scene_sdf(vec3 p)
{
    vec3 m = vec3(5);
    p.xz = mod(p.xz, m.xz);
    float d = length(p - 0.5 * m) - 0.5;
    return d;
}

void
main(void)
{
    vec3 eye = vec3(0, 0, 0);
    float fov = 45.0;

    // Calculate the view direction
    vec2 xy = gl_FragCoord.xy - size / 2.0;
    float z = size.y / tan(radians(fov) / 2.0);
    vec3 dir = normalize(vec3(xy, -z));

	// Calculate the shortest distance to a surface
    int step = 0;
    float depth = MIN_DIST;
    for (step = 0; step < MAX_MARCHING_STEPS; step++) {
    	vec3 p = vec3(view * vec4(eye + depth * dir, 1.0f));
        float dist = scene_sdf(p);
        if (dist < EPSILON) {
            break;
        }

        depth += dist;
        if (depth >= MAX_DIST) {
            depth = MAX_DIST;
            break;
        }
    }

    if (step == MAX_MARCHING_STEPS || depth > MAX_DIST - EPSILON) {
        out_color = vec4(vec3(0), 1);
        return;
    }

	vec3 p = vec3(view * vec4(eye + depth * dir, 1.0f));

	// Calculate the normal vector
	vec3 normal;
	normal.x = scene_sdf(vec3(p.x + EPSILON, p.y, p.z)) - scene_sdf(vec3(p.x - EPSILON, p.y, p.z));
	normal.y = scene_sdf(vec3(p.x, p.y + EPSILON, p.z)) - scene_sdf(vec3(p.x, p.y - EPSILON, p.z));
	normal.z = scene_sdf(vec3(p.x, p.y, p.z + EPSILON)) - scene_sdf(vec3(p.x, p.y, p.z - EPSILON));
	normal = normalize(normal);

	vec3 light_dir = normalize(vec3(1, 1, 1));
	float r = dot(light_dir, normal);

    out_color = vec4(r, 0, 0, 1);
}
