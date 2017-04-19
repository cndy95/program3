
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
const int MAXLIGHTS = 4;
uniform LightProperties Lights[MAXLIGHTS];

// material description
uniform vec3 ambient;	
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;
uniform int enableTexture;

uniform sampler2D textVar;

in vec2 textCoord;
in vec3 Normal;		// normal in eye coordinates
in vec4 Position;	// vertex position in eye coordinates

out vec4 FragColor;

void main()
{
	vec3 scatteredLight = vec3(0.f);
	vec3 reflectedLight = vec3(0.f);
	vec3 eyeDirection;
	vec3 lightDirection;
	vec3 halfVector;
	vec3 myNormal;
	float attenuation = 1.0f;
	float diffuseCoeff;
	float specularCoeff;

    // loop over all the lights
    for (int light = 0; light < MAXLIGHTS; ++light) 
	{
        if (Lights[light].isEnabled == 0)
            continue;

        attenuation = 1.0;

        eyeDirection = normalize(-vec3(Position));	// since we are in eye coordinates
													// eye position is 0,0,0
        // for local lights, compute per-fragment direction,
        // halfVector, and attenuation
        if (Lights[light].isLocal == 1) 
		{
		    lightDirection = Lights[light].position - vec3(Position);
            float lightDistance = length(lightDirection);
            lightDirection = normalize(lightDirection);

			attenuation = 1.0 /
				(Lights[light].constantAttenuation
				+ Lights[light].linearAttenuation    * lightDistance
				+ Lights[light].quadraticAttenuation * lightDistance
						                                  * lightDistance);
				if (Lights[light].isSpot == 1) 
				{
					vec3 myConeDirection = normalize(Lights[light].coneDirection);
					float spotCos = dot(lightDirection,
										-myConeDirection);
					if (spotCos < Lights[light].spotCosCutoff)
						attenuation = 0.0;
					else 
						attenuation *= pow(spotCos, Lights[light].spotExponent);
				}
            halfVector = normalize(lightDirection + eyeDirection);
        } 
		else
		// directional light
		{
			lightDirection = normalize(Lights[light].position);
			halfVector = normalize(lightDirection + eyeDirection);
        }

		myNormal = normalize(Normal);

        diffuseCoeff  = max(0.0, dot(myNormal, lightDirection));
        specularCoeff = max(0.0, dot(myNormal, halfVector));

        if (diffuseCoeff == 0.0)
            specularCoeff = 0.0;
        else 
            specularCoeff = pow(specularCoeff, shininess); // * Strength;

		// Accumulate all the lights’ effects as it interacts with material properties

		scatteredLight += Lights[light].ambient * ambient * attenuation +
                          Lights[light].color * (diffuseCoeff * diffuse) * attenuation;
        reflectedLight += Lights[light].color * (specularCoeff * specular) * attenuation;

    }

	vec3 rgb = min(scatteredLight + reflectedLight, vec3(1.0));

	if(enableTexture == 1){

		vec4 color = texture(textVar,textCoord);	
		if (color.x == 0)
			discard;	
		else
			FragColor = color*vec4(rgb,1.0f);
	}
	else{
		FragColor = vec4(rgb,1.0f);
	}

}