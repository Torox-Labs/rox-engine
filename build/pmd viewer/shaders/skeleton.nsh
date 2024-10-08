@sampler bones "bones"

@vertex
uniform sampler2D bones;

vec3 tr(vec3 p,float idx)
{
    vec4 q=texture2D(bones,vec2(idx,0.75));
    return texture2D(bones,vec2(idx,0.25)).xyz+p+cross(q.xyz,cross(q.xyz,p)+p*q.w)*2.0;
}

vec3 trn(vec3 n,float idx)
{
    vec4 q=texture2D(bones,vec2(idx,0.75));
    return n+cross(q.xyz,cross(q.xyz,n)+n*q.w)*2.0;
}
