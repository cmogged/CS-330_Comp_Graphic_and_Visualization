#version 440 core

struct Material 
{
    vec3 ambientColor;
    float ambientStrength;
    vec3 diffuseColor;
    vec3 specularColor;
    float shininess;
}; 

struct LightSource 
{
    vec3 position;	
    vec3 ambientColor;
    vec3 diffuseColor;
    vec3 specularColor;
    float focalStrength;
    float specularIntensity;
};

#define TOTAL_LIGHTS 1

in vec3 fragmentPosition;
in vec3 fragmentVertexNormal;
in vec2 fragmentTextureCoordinate;

out vec4 outFragmentColor;

uniform bool bUseTexture=false;
uniform bool bUseLighting=false;
uniform vec4 objectColor = vec4(1.0f);
uniform sampler2D objectTexture;
uniform vec3 viewPosition;
uniform vec2 UVscale = vec2(1.0f, 1.0f);
uniform LightSource lightSources[TOTAL_LIGHTS];
uniform Material material;

// function prototypes
vec3 CalcLightSource(LightSource light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
   if(bUseLighting == true)
   {
      // properties
      vec3 lightNormal = normalize(fragmentVertexNormal);
      vec3 viewDirection = normalize(viewPosition - fragmentPosition);
      vec3 phongResult = vec3(0.0f);

      for(int i = 0; i < TOTAL_LIGHTS; i++)
      {
         phongResult += CalcLightSource(lightSources[i], lightNormal, fragmentPosition, viewDirection);
      }
      
      if(bUseTexture == true)
      {
         vec4 textureColor = texture(objectTexture, fragmentTextureCoordinate * UVscale);
         outFragmentColor = vec4(phongResult * textureColor.xyz, 1.0);
      }
      else
      {
         outFragmentColor = vec4(phongResult * objectColor.xyz, objectColor.w);
      }
   }
   else 
   {
      if(bUseTexture == true)
      {
         outFragmentColor = texture(objectTexture, fragmentTextureCoordinate * UVscale);
      }
      else
      {
         outFragmentColor = objectColor;
      }
   }
}

// taken and modified from https://opentk.net/learn/chapter2/6-multiple-lights.html
vec3 CalcLightSource(LightSource light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    // values for attenuation
    //#define light_constant 0.02
    //#define light_linear 0.1
    //#define light_quadratic 0.1

    vec3 lightDir = normalize(light.position - fragPos);
    
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    // attenuation
    //float distance = length(light.position - fragPos);
    //float attenuation = 1.0 / (light_constant + light_linear * distance + light_quadratic * (distance * distance));
    
    // combine results
    vec3 ambient  = light.ambientColor * vec3(material.diffuseColor);
    vec3 diffuse  = light.diffuseColor * diff * vec3(material.diffuseColor);
    vec3 specular = light.specularColor * spec * vec3(material.specularColor) * light.specularIntensity;
    
    //ambient  *= attenuation;
    //diffuse  *= attenuation;
    //specular *= attenuation;
    
    return (ambient + diffuse + specular);
}
