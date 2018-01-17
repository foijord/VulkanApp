#version 450

layout(std140, binding = 0) uniform Transform {
  mat4 ModelViewMatrix;
  mat4 ProjectionMatrix;
};

layout(std140, binding = 1) uniform ShaderParams {
  vec3 Material;
  float DensityFactor;
  vec3 LightPosition;
  float LightDensityFactor;
  vec3 LightIntensity;
  int NumDensitySteps;
  int NumLightSteps;
};

layout(set = 0, binding = 3) uniform sampler3D Density;

layout(location = 0) in vec3 vertex;
layout(location = 0) out vec4 FragColor;

bool IntersectBox(in vec3 pos, in vec3 dir, out float t0, out float t1)
{
  vec3 invR = 1.0 / dir;
  vec3 tbot = invR * (vec3(0) - pos);
  vec3 ttop = invR * (vec3(1) - pos);
  vec3 tmin = min(ttop, tbot);
  vec3 tmax = max(ttop, tbot);
  vec2 t = max(tmin.xx, tmin.yz);
  t0 = max(t.x, t.y);
  t = min(tmax.xx, tmax.yz);
  t1 = min(t.x, t.y);
  return t0 <= t1;
}

vec4 integrate(in vec3 raypos, in vec3 stepdir, in float absorption)
{
  float t0, t1;
  IntersectBox(raypos, stepdir, t0, t1);

  vec3 stop = raypos + stepdir * t1;
  
  float ds = length(stop - raypos) / NumDensitySteps;
  float dl = sqrt(2) / NumLightSteps;

  float Ttotal = 1;
  vec3  Ctotal = vec3(0.1);
  float alpha = 0;
  
  for (int step = 0; step < NumDensitySteps; ++step) {
    raypos += stepdir * ds;
    float rho = texture(Density, raypos).r * DensityFactor;
    if (rho <= 0.0)
      continue;
    
    float Ti = exp(-absorption * rho * ds);
    Ttotal *= Ti;
    if (Ttotal < 0.01)
      break;

    float lTtotal = 1.0;
    vec3 lraypos = raypos;
    vec3 lstepdir = normalize(LightPosition - lraypos);
    for (int lstep = 0; lstep < NumLightSteps; ++lstep) {
      lraypos += lstepdir * dl;
      float lrho = texture(Density, lraypos).r * LightDensityFactor;
      if (lrho <= 0.0)
        continue;
      
      float lTi = exp(-absorption * lrho * dl);
      lTtotal *= lTi;
      if (lTtotal < 0.01)
        break;
    }
    vec3 Ci = Ttotal * lTtotal * Material * LightIntensity * rho * ds;
    Ctotal += Ci;
    alpha += (1 - Ti) * (1 - alpha);
  }

  return vec4(Ctotal, alpha);
}

void main()
{
  ivec3 size = textureSize(Density, 0);
  
  const float maxDist = sqrt(2.0);
  float dt = maxDist / float(size.x);

  vec3 eye = vec3(inverse(ModelViewMatrix) * vec4(0, 0, 0, 1));
  vec3 dir = normalize(vertex - eye);

  FragColor = integrate(vertex, dir, 2);
}
