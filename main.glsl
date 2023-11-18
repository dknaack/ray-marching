out v4 out_color;

uniform v2 size;
uniform f32 time;
uniform m4x4 view;

const i32 MAX_MARCHING_STEPS = 100;
const f32 MAX_DIST = 1000.0;
const f32 MIN_DIST = 0.0;
const f32 EPSILON = 0.0001;

f32
scene_sdf(v3 p)
{
    v3 m = v3(5);
    p.xz = mod(p.xz, m.xz);
    f32 d = length(p - 0.5 * m) - 0.5;
    return d;
}

void
main(void)
{
    v3 eye = v3(0, 0, 0);
    f32 fov = 45.0;

    // Calculate the view direction
    v2 xy = gl_FragCoord.xy - size / 2.0;
    f32 z = size.y / tan(radians(fov) / 2.0);
    v3 dir = normalize(v3(xy, -z));

	// Calculate the shortest distance to a surface
    i32 step = 0;
    f32 depth = MIN_DIST;
    for (step = 0; step < MAX_MARCHING_STEPS; step++) {
    	v3 p = v3(view * v4(eye + depth * dir, 1.0f));
        f32 dist = scene_sdf(p);
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
        out_color = v4(v3(0), 1);
        return;
    }

	v3 p = v3(view * v4(eye + depth * dir, 1.0f));

	// Calculate the normal vector
	v3 normal;
	normal.x = scene_sdf(v3(p.x + EPSILON, p.y, p.z)) - scene_sdf(v3(p.x - EPSILON, p.y, p.z));
	normal.y = scene_sdf(v3(p.x, p.y + EPSILON, p.z)) - scene_sdf(v3(p.x, p.y - EPSILON, p.z));
	normal.z = scene_sdf(v3(p.x, p.y, p.z + EPSILON)) - scene_sdf(v3(p.x, p.y, p.z - EPSILON));
	normal = normalize(normal);

	v3 light_dir = normalize(v3(1, 1, 1));
	f32 r = dot(light_dir, normal);

    out_color = v4(r, 0, 0, 1);
}
