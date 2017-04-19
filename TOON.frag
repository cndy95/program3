// Fragment shader for multiple lights.

#version 410 core

struct LightProperties {
    int isEnabled;
    int isLocal;
    int isSpot;
    vec3 ambient;
    vec3 color;
    vec3 position;
    vec3 halfVector;
    vec3 coneDirection;
    float spotCosCutoff;
    float spotExponent;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
};

// the set of lights to apply, per invocation of this shader
uniform LightProperties Lights[4];

// material description
uniform vec3 ambient;	
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

in vec3 Normal;		// normal in eye coordinates
in vec4 Position;	// vertex position in eye coordinates

out vec4 FragColor;

void main()
{
	vec3 lightDirection;
	vec3 myNormal;
	float diffuseCoeff;

    int maxlight = 1;
    vec3 rgb;
    for (int light = 0; light < maxlight; ++light) 
	{	
		if (Lights[light].isSpot == 1) 
		{
			Lights[light].isEnabled == 0;
			maxlight = maxlight + 1;
			
		}
        if (Lights[light].isEnabled == 0)
            continue;


        if (Lights[light].isLocal == 1) 
		{
		    lightDirection = normalize(Lights[light].position - vec3(Position));
        } 
		else
		{
			lightDirection = normalize(Lights[light].position);
        }

		myNormal = normalize(Normal);

        diffuseCoeff  = max(0.0, dot(myNormal, lightDirection));      

        rgb = Lights[light].color * ambient + Lights[light].color * diffuseCoeff * diffuse;

    }
    float intensity = dot(lightDirection,normalize(Normal));
	if (intensity > 0.86)
		intensity = 0.86;
	else if (intensity > 0.86)
		intensity = 0.86;
	else if (intensity > 0.71)
		intensity = 0.71;
	else if (intensity > 0.57)
		intensity = 0.57;
	else if (intensity > 0.43)
		intensity = 0.43;
	else if (intensity > 0.29)
		intensity = 0.29;
	else if (intensity > 0.14)
		intensity = 0.14;
	else
		intensity = 0;
	rgb = rgb * intensity;
    FragColor = vec4(rgb,1.0f);
}