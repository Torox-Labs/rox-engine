@all

varying vec2 tc;
varying vec2 rtc;
varying vec3 view_ray;

@vertex

uniform vec4 vp;

void main()
{
    tc=gl_MultiTexCoord0.xy;
    rtc=tc*(vp.zw/128.0);

    float tan_half_fov=-1.0/gl_ProjectionMatrix[1][1];
    view_ray = vec3(gl_Vertex.xy*tan_half_fov,1.0);
    view_ray.x*=vp.z/vp.w;

    gl_Position=gl_Vertex;
}

@sampler depth_map "depth"
@sampler rand_map "random"
@predefined vp "nya viewport"

@procedural kernel = normalize3(rand2(-1.0,1.0),rand2(-1.0,1.0),rand2(0.0,1.0)) * lerp(0.1,1.0,(i/count)^2.0)

@fragment

uniform sampler2D depth_map;
uniform sampler2D rand_map;

uniform vec4 vp;
uniform vec4 kernel[32];

vec3 normal_from_depth(float depth, vec2 tc)
{
    vec2 off1=vec2(1.0/vp.z,0.0);
    vec2 off2=vec2(0.0,1.0/vp.w);

    float depth1=texture2D(depth_map,tc+off1).r;
    float depth2=texture2D(depth_map,tc+off2).r;
  
    vec3 p1=vec3(off1,depth1-depth);
    vec3 p2=vec3(off2,depth2-depth);

    vec3 normal=cross(p1,p2);
    normal.z=-normal.z;
    return normalize(normal);
}

void main()
{
    float depth=texture2D(depth_map,tc).r;

    vec3 pos = view_ray * depth;
    vec3 normal = normal_from_depth(depth,tc);
    vec3 rvec = texture2D(rand_map,rtc).xyz * 2.0 - 1.0;

    vec3 tangent = normalize(rvec - dot(rvec, normal) * normal);
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal);

    float radius = 1.5;

    float occlusion = 0.0;
    for (int i = 0; i < 32; ++i)
    {
       vec3 sample = tbn * kernel[i].xyz;
       sample = sample * radius + pos;

       vec4 offset = vec4(sample, 0.0);
       offset = gl_ProjectionMatrix * offset;
       offset.xy /= offset.w;
       offset.xy = offset.xy * 0.5 + 0.5;

       float sample_depth = texture2D(depth_map,offset.xy).r;

       float range_check = abs(pos.z - sample_depth) < radius ? 1.0 : 0.0;
       occlusion += step(sample_depth,sample.z) * range_check;
    }

    occlusion = 1.0 - occlusion / 32.0;

    gl_FragColor=vec4(occlusion);
}
