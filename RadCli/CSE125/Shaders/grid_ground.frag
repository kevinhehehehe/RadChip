#version 400

/* UNCOMMENT FOR FOG EFFECT (1/2)*/
/*struct FogInfo{
	float maxDist;
	float minDist;
	vec3 color;
};
uniform FogInfo fog;*/

struct Light{
	int type;//0:directional  1:point  2:spot
	vec4 pos;
	vec4 dir;
	float spotCutOff;//cos(a)
	vec3 specular;
	vec3 diffuse;
	vec3 ambient;
};

uniform Light light[1];//one light for basic shader. To add light, increment light array size, then update ads()

/* UNCOMMENT FOR SKY BOX REFLECTION (1/2)*/
/*uniform samplerCube CubeMapTex;//sky box texture // for reflection */

uniform vec3 color;
uniform float width;

uniform	sampler2D colorTex;
uniform sampler2D occlusionTex;
uniform sampler2D specularTex;
uniform sampler2D normalTex;

in vec3 position;//position in world
in vec3 normal;//normal in world
in vec2 texCor;
flat in vec3 cam;//camera in world

layout (location=0) out vec4 FragColor;

vec3 norm;//norm based on facing, decided in main()

vec3 texDiffuse;
vec3 texSpecular;
vec3 texOcc;

vec4 myads(){
	vec4 ads=vec4(0.0,0.0,0.0,1.0);
	vec3 n = normalize(norm);
	vec3 s,v,r;

	//light[0]
	if(light[0].type==0){
		s = normalize( vec3(light[0].pos));
	}else{
		s = normalize( vec3(light[0].pos)-position);
	}
	v = normalize(vec3(cam-position));
	r = reflect(-s,n);
	if( light[0].type!=2 ){//direction light and point light
		ads += max( vec4(light[0].ambient*texDiffuse,1.0), 
		            vec4(light[0].diffuse*texDiffuse*max(dot(s,n),0.0),1.0) 
		            + vec4(light[0].specular*texSpecular*pow(max(dot(r,v),0.0), 10),1.0)); //Shininess = 10
	}else if( light[0].type==2 && dot(normalize(vec3(-light[0].dir)),s) >= light[0].spotCutOff){//spot light
		ads += (dot(normalize(vec3(-light[0].dir)),s)-light[0].spotCutOff)/(1-light[0].spotCutOff)//to make soft edge
			   * max( vec4(light[0].ambient*texDiffuse,1.0),  
			          vec4(light[0].diffuse*texDiffuse*max(dot(s,n),0.0),1.0) 
			          + vec4(light[0].specular*texSpecular*pow(max(dot(r,v),0.0), 10),1.0)); //Shininess = 10
	}

	return vec4(ads.xyz*texOcc.xyz,1.0);
	//return ads;
}

void main()
{
	vec4 nt = texture(normalTex, texCor);
	vec3 normal = 2*vec3(nt[0],nt[2],nt[1])-vec3(1.0,1.0,1.0);//normal = vec3(ModelMatrix*vec4(VertexNormal,0));

	if(gl_FrontFacing){
		norm = normal;
	}else{
		norm = -normal;
	}

	vec4 ct = texture(colorTex,texCor);
	vec4 st = texture(specularTex,texCor);
	vec4 ot = texture(occlusionTex,texCor);

	texDiffuse = ct.xyz;
	texSpecular = st.xyz;
	texOcc = ot.xyz;

	FragColor = myads(); 

	//FragColor = vec4(normal,1.0);

	/* UNCOMMENT FOR SKYBOX REFLECTION (2/2) */
	/* //apply skybox reflection
	vec3 reflectDir = reflect(position-cam,normalize(norm));
	vec4 cubeMapColor = texture(CubeMapTex,reflectDir);
	ads = mix(ads,cubeMapColor,material.ReflectFactor);
	//ads = mix(ads,cubeMapColor,1-(1-material.ReflectFactor)*dot(normalize(reflectDir),normalize(norm))); for more complex reflect */

	/* UNCOMMENT FOR FOG EFFECT (2/2)*/
	/* //apply fog
	float dist = distance(position,cam);
	float fog_factor = (fog.maxDist-dist)/(fog.maxDist-fog.minDist);
	fog_factor = clamp(fog_factor,0.0,1.0);
	ads = mix(vec4(fog.color,1.0),ads,fog_factor); */
}