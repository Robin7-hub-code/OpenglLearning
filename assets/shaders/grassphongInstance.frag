#version 460 core
out vec4 FragColor;

in vec2 uv;
in vec3 normal;
in vec3 worldPosition;
in vec2 worldXZ;

uniform sampler2D sampler;	//diffuse贴图采样器
uniform sampler2D specularMaskSampler;//specularMask贴图采样器
uniform sampler2D opacityMask;
uniform sampler2D cloudMask;

uniform vec3 cloudBlack;
uniform vec3 cloudWhite;
uniform float cloudUvScale;
uniform vec3 windDir;
uniform float lerp_para;

uniform vec3 ambientColor;

//相机世界位置
uniform vec3 cameraPosition;

//草地亮度
uniform float shiness;
uniform float time;
//云朵运动速度
uniform float CloudSpeed;


//透明度
uniform float opacity;
//草uv
uniform float uvScale;
uniform float brightness;
struct DirectionalLight{
	vec3 direction;
	vec3 color;
	float specularIntensity;
};

struct PointLight{
	vec3 position;
	vec3 color;
	float specularIntensity;

	float k2;
	float k1;
	float kc;
};

struct SpotLight{
	vec3 position;
	vec3 targetDirection;
	vec3 color;
	float outerLine;
	float innerLine;
	float specularIntensity;
};

uniform DirectionalLight directionalLight;

//计算漫反射光照
vec3 calculateDiffuse(vec3 lightColor, vec3 objectColor, vec3 lightDir, vec3 normal){
	float diffuse = clamp(dot(-lightDir, normal), 0.0,1.0);
	vec3 diffuseColor = lightColor * diffuse * objectColor;

	return diffuseColor;
}

//计算镜面反射光照
vec3 calculateSpecular(vec3 lightColor, vec3 lightDir, vec3 normal, vec3 viewDir, float intensity){
	//1 防止背面光效果
	float dotResult = dot(-lightDir, normal);
	float flag = step(0.0, dotResult);
	vec3 lightReflect = normalize(reflect(lightDir,normal));

	//2 jisuan specular
	float specular = max(dot(lightReflect,-viewDir), 0.0);

	//3 控制光斑大小
	specular = pow(specular, shiness);

	//float specularMask = texture(specularMaskSampler, uv).r;

	//4 计算最终颜色
	vec3 specularColor = lightColor * specular * flag * intensity;

	return specularColor;
}

vec3 calculateSpotLight(SpotLight light, vec3 normal, vec3 viewDir){
	//计算光照的通用数据
	vec3 objectColor  = texture(sampler, uv).xyz;
	vec3 lightDir = normalize(worldPosition - light.position);
	vec3 targetDir = normalize(light.targetDirection);

	//计算spotlight的照射范围
	float cGamma = dot(lightDir, targetDir);
	float intensity =clamp( (cGamma - light.outerLine) / (light.innerLine - light.outerLine), 0.0, 1.0);

	//1 计算diffuse
	vec3 diffuseColor = calculateDiffuse(light.color,objectColor, lightDir,normal);

	//2 计算specular
	vec3 specularColor = calculateSpecular(light.color, lightDir,normal, viewDir,light.specularIntensity); 

	return (diffuseColor + specularColor)*intensity;
}

vec3 calculateDirectionalLight(vec3 objectColor,DirectionalLight light, vec3 normal ,vec3 viewDir){
	//计算光照的通用数据
	
	vec3 lightDir = normalize(light.direction);

	//1 计算diffuse
	vec3 diffuseColor = calculateDiffuse(light.color,objectColor, lightDir,normal);

	//2 计算specular
	vec3 specularColor = calculateSpecular(light.color, lightDir,normal, viewDir,light.specularIntensity); 

	return diffuseColor + specularColor;
}

vec3 calculatePointLight(vec3 objectColor,PointLight light, vec3 normal ,vec3 viewDir){
	//计算光照的通用数据

	vec3 lightDir = normalize(worldPosition - light.position);

	//计算衰减
	float dist = length(worldPosition - light.position);
	float attenuation = 1.0 / (light.k2 * dist * dist + light.k1 * dist + light.kc);

	//1 计算diffuse
	vec3 diffuseColor = calculateDiffuse(light.color,objectColor, lightDir,normal);

	//2 计算specular
	vec3 specularColor = calculateSpecular(light.color, lightDir,normal, viewDir,light.specularIntensity); 

	return (diffuseColor + specularColor)*attenuation;
}

void main()
{
	
	vec2 worldUV=worldXZ/uvScale;
	vec3 objectColor  = texture(sampler,worldUV).xyz*brightness;
	vec3 result = vec3(0.0,0.0,0.0);

	//计算光照的通用数据
	vec3 normalN = normalize(normal);
	vec3 viewDir = normalize(worldPosition - cameraPosition);

	result += calculateDirectionalLight(objectColor,directionalLight,normalN, viewDir);

	//环境光计算
	
	float alpha =  texture(opacityMask, uv).r;

	vec3 ambientColor = objectColor * ambientColor;

	vec3 grassColor = result + ambientColor;

	vec2 cloudUV=worldXZ/cloudUvScale;
	vec2 wind_cloudDir=normalize(windDir.xz);

	cloudUV=cloudUV+time*CloudSpeed*wind_cloudDir;
	float cloudMask=texture(cloudMask,cloudUV).x;
	vec3 cloudColor=mix(cloudBlack,cloudWhite,cloudMask);

	vec3 finalColor=mix(cloudColor,grassColor,lerp_para);

	FragColor = vec4(finalColor,alpha * opacity);
}