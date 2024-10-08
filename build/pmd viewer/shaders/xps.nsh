@sampler base "diffuse"
@sampler bump "bump"
@sampler lightmap "lightmap"
@sampler spec "spec"
@sampler env "env"

@uniform alpha_test "alpha test"
@predefined cam_pos "nya camera position":local
@uniform light_dir "light dir":local_rot=-0.58,0.58,0.57
@uniform light_k "light k"=0.1,0.0,0.0,0.0

@predefined bones_pos_map "nya bones pos transform texture"=4096
@predefined bones_rot_map "nya bones rot texture"=4096

@all

varying vec3 pos;
varying vec2 tc;
varying mat3 tbn;
varying vec4 color;

@vertex

uniform sampler2D bones_pos_map;
uniform sampler2D bones_rot_map;

vec3 rotate(vec3 v,vec4 q) { return v+cross(q.xyz,cross(q.xyz,v)+v*q.w)*2.0; }

void main()
{
    tc=gl_MultiTexCoord0.xy;
    color=gl_Color;

    vec2 tc=vec2(gl_MultiTexCoord6[0],0.0);
    vec4 q=texture2D(bones_rot_map,tc);
    pos=(rotate(gl_Vertex.xyz,q)+texture2D(bones_pos_map,tc).xyz)*gl_MultiTexCoord7[0];
    vec3 n=rotate(gl_Normal,q)*gl_MultiTexCoord7[0];
    vec3 t=rotate(gl_MultiTexCoord4.xyz,q)*gl_MultiTexCoord7[0];

    for(int i=1;i<4;++i)
    {
        if(gl_MultiTexCoord7[i]>0.0)
        {
            tc=vec2(gl_MultiTexCoord6[i],0.0);
            q=texture2D(bones_rot_map,tc);
            pos+=(rotate(gl_Vertex.xyz,q)+texture2D(bones_pos_map,tc).xyz)*gl_MultiTexCoord7[i];
            n+=rotate(gl_Normal,q)*gl_MultiTexCoord7[i];
            t+=rotate(gl_MultiTexCoord4.xyz,q)*gl_MultiTexCoord7[i];
        }
    }

    tbn=mat3(t,cross(t,n),n);

    gl_Position=gl_ModelViewProjectionMatrix*vec4(pos,gl_Vertex.w);
}

@fragment

uniform sampler2D base;
uniform sampler2D bump;
uniform sampler2D lightmap;
uniform sampler2D spec;
uniform sampler2D env;

uniform vec4 alpha_test;
uniform vec4 cam_pos;
uniform vec4 light_dir;
uniform vec4 light_k;

void main()
{
    vec4 c=texture2D(base,tc);
    if(c.a*alpha_test.x+alpha_test.y>0.0)
        discard;

    vec3 normal=normalize(tbn*(texture2D(bump,tc).xyz*2.0-1.0));

    c.rgb*=light_k.x+light_k.y*max(0.0,dot(normal,light_dir.xyz));

    vec3 v=normalize(pos-cam_pos.xyz);
    vec3 lrn=reflect(light_dir.xyz,normal);

    c.rgb+=pow(max(0.0,dot(v,lrn)),10.0)*light_k.z*texture2D(spec,tc).rgb;
    
    c.rgb*=texture2D(lightmap,tc).rgb;

    c*=color;

    vec3 r=normalize(reflect(v,normal));
	vec2 rtc = 0.5*r.xy/length(r)+0.5;
    c.rgb+=texture2D(env,rtc).rgb*0.3;

    gl_FragColor=c;
}
