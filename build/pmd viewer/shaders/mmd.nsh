@sampler base "diffuse"
@sampler toon "toon"
@sampler env "env"
@uniform env_p "env param"

@uniform light_dir "light dir"=-0.4,0.82,0.4
@uniform light_color "light color"=0.6,0.6,0.6
@predefined cam_pos "nya camera position":local
@predefined cam_rot "nya camera rotation":local

@uniform amb_k "amb k"
@uniform diff_k "diff k"
@uniform spec_k "spec k"

@all

varying vec2 tc;
varying vec2 env_tc;
varying vec3 normal;
varying vec3 pos;

@vertex
uniform vec4 cam_rot;

void main()
{
    tc=gl_MultiTexCoord0.xy;
    pos=gl_Vertex.xyz;
    normal=gl_Normal.xyz;
    env_tc=0.5*normal.xy+0.5+cross(cross(normal.xyz,cam_rot.xyz)+normal.xyz*cam_rot.w,cam_rot.xyz).xy;

    gl_Position=gl_ModelViewProjectionMatrix*vec4(pos,1.0);
}

@fragment
uniform sampler2D base;
uniform sampler2D toon;
uniform sampler2D env;
uniform vec4 env_p;

uniform vec4 light_dir;
uniform vec4 light_color;
uniform vec4 cam_pos;

uniform vec4 amb_k;
uniform vec4 diff_k;
uniform vec4 spec_k;

void main()
{
    vec4 c = texture2D(base,tc);
    c.a *= diff_k.a;
    if(c.a < 0.01)
        discard;

    float s = 0.0;
    float l = 0.5 + 0.5 * dot(light_dir.xyz, normal);
    vec3 t = texture2D(toon,vec2(s, l)).rgb;

    vec3 eye = normalize(cam_pos.xyz - pos);
    float ndh = dot(normal, normalize(light_dir.xyz + eye));
    vec3 spec = spec_k.rgb * max(pow(ndh, spec_k.a), 0.0);

    c.rgb *= clamp(amb_k.rgb + (diff_k.rgb) * light_color.rgb, vec3(0.0), vec3(1.0));
    c.rgb += spec * light_color.rgb;
    c.rgb *= t;

    vec4 e = texture2D(env, env_tc);
    c=mix(c,c*e,env_p.x);
    c.rgb+=env_p.y*e.rgb;

    gl_FragColor = c;
}
