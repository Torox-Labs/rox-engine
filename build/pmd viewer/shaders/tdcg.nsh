@sampler base "ColorTex"
@sampler shade "ShadeTex"

@uniform light_dir "light dir"=0.3,-0.4,-0.3

@uniform ambient "Ambient"

@uniform color_blend "ColorBlend"=10.0
@uniform shade_blend "ShadeBlend"=10.0

@uniform high_light "HighLight"
@uniform high_light_power "HighLightPower"=100.0
@uniform high_light_blend "HighLightBlend"=10.0

@all

varying vec2 tc;
varying vec3 normal;

@vertex

void main()
{
    tc=gl_MultiTexCoord0.xy;
    normal=gl_Normal;
    gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;
}

@fragment

uniform sampler2D base;
uniform sampler2D shade;

uniform vec4 light_dir;

uniform vec4 ambient;

uniform vec4 color_blend;
uniform vec4 shade_blend;

uniform vec4 high_light;
uniform vec4 high_light_power;
uniform vec4 high_light_blend;

void main()
{
    float l=dot(normal,-light_dir.xyz);
    float lp=min(1.0,max(0.0,l+ambient.r*0.01));
    lp=clamp(lp,0.05,0.95); //ToDo: clamp to edge instead
	float hp=pow(min(1.0,max(0.0,l+high_light.r*0.01)),high_light_power.r);

	vec4 texcol=texture2D(base,tc);
    vec4 shadecol=texture2D(shade,vec2(lp,0.5));

	vec4 hl=vec4(vec3(hp),1.0);

	vec4 col=(texcol*color_blend.r*0.1)*(shadecol.r*shade_blend.r*0.1);
	col+=hl*high_light_blend.r*0.1;

	gl_FragColor=vec4(col.rgb,texcol.a);
}
