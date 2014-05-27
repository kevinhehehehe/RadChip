#version 400   

// Attributes
layout (location=0) in float a_pID;
layout (location=1) in float a_pRadiusOffset;
layout (location=2) in float a_pVelocityOffset;
layout (location=3) in float a_pDecayOffset;
layout (location=4) in float a_pSizeOffset;
layout (location=5) in vec3 a_pColorOffset;

// Uniforms
uniform mat4	u_ViewMatrix;
uniform mat4	u_ProjectionMatrix;
uniform mat4	u_ModelMatrix; 
uniform vec2	u_Gravity;
uniform float   u_Time;
uniform float   u_eRadius;
uniform float   u_eVelocity;
uniform float   u_eDecay;
uniform float   u_eSize;

// Varying
out vec3		v_pColorOffset;

out vec3 position;//position in world
flat out vec3 cam;//camera in world

void main(void)
{
	// 1
	// Convert polar angle to cartesian coordinates and calculate radius
	float x = cos(a_pID);
	float y = sin(a_pID);
	float r = u_eRadius * a_pRadiusOffset;

	// 2
	// Lifetime
	float growth = r / (u_eVelocity + a_pVelocityOffset);
	float decay = u_eDecay + a_pDecayOffset;

	// 3
	// If blast is growing
	if(u_Time < growth)
	{
		float time = u_Time / growth;
		x = x * r * time;
		y = y * r * time;
	}

	// 4
	// Else if blast is decaying
	else
	{
		float time = (u_Time - growth) / decay;
		x = (x * r) + (u_Gravity.x * time);
		y = (y * r) + (u_Gravity.y * time);
	}

	// 5
	// Required OpenGLES 2.0 outputs
	
	vec4 pos = vec4(x, y, 0.0, 1.0);
	cam = vec3(inverse(u_ViewMatrix)*vec4(0,0,0,1));
	
	position = vec3(u_ModelMatrix*pos);
	
	gl_Position = u_ProjectionMatrix*u_ViewMatrix*u_ModelMatrix*pos;
	//gl_Position = u_ProjectionMatrix * vec4(x, y, 0.0, 1.0);
	gl_PointSize = max(0.0, (u_eSize + a_pSizeOffset));
 
	// Fragment Shader outputs
	v_pColorOffset = a_pColorOffset;
}                                                                         
