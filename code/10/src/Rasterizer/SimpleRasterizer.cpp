#include <algorithm>
#include <iostream>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/matrix_inverse.hpp>

#define RAYTRACER_USE_FOREACH
#include <Raytracer/Raytracer.h>

#include <Rasterizer/SimpleRasterizer.h>

using namespace std;
using namespace glm;
using namespace Rasterizer;
using namespace Raytracer;
using namespace Raytracer::Objects;
using namespace Raytracer::Scenes;

SimpleRasterizer::SimpleRasterizer()
{
  ambientLight = vec3(0.01f);
}

bool SimpleRasterizer::CompareTriangle(const Triangle &t1, const Triangle &t2)
{
  // These aren't actually the mean values, but since both are off by a constant factor (3),
  // this inequation is equivalent.
  return (t1.position[0].z + t1.position[1].z + t1.position[2].z >
    t2.position[0].z + t2.position[1].z + t2.position[2].z);
}

void SimpleRasterizer::DrawSpan(int x1, int x2, int y, float z1, float z2, vec3 &color1,
                vec3 &color2)
{
  if (x1 < x2 && (x1 > 0) && (x2 < image->GetWidth()) && (y > 0) && (y < image->GetWidth())) 
    for (int x = x1; x < x2; x++) {
      image->SetPixel(x, y, color1);
    }
}

void SimpleRasterizer::DrawTriangle(const Triangle &t)
{
  for (int i = 0; i < 3; ++i)
  {
    int x = (int)t.position[i].x;
    int y = (int)t.position[i].y;
    if ((x > 0) && (x < image->GetWidth()) && (y > 0) && (y < image->GetHeight()))
    {
      image->SetPixel(x, y, t.color[i]);
    }
  }
  // sort the vertices from topmost to bottommost s.t. :
  // i < j <=> t.position[sorted[i]].y <= t.position[sorted[j]].y, 
  // initially an unsorted array
  int sorted[3] = {0, 1, 2};
  // selection sort on y cooridnate
  for (int i = 0; i < 2; i++) {
    int j = i;
    for (int k = i + 1; k < 3; k++) {
      if (t.position[sorted[k]].y < t.position[sorted[j]].y)
        j = k;
    }
    int temp = sorted[i];
    sorted[i] = sorted[j];
    sorted[j] = temp;
  }

  std::cout << "P0.y = " << t.position[0].y << std::endl;
  std::cout << "P1.y = " << t.position[1].y << std::endl;
  std::cout << "P2.y = " << t.position[2].y << std::endl;

  std::cout << "sorted[0] = " << sorted[0] << std::endl;
  std::cout << "sorted[1] = " << sorted[1] << std::endl;
  std::cout << "sorted[2] = " << sorted[2] << std::endl;

  float y_top = t.position[sorted[0]].y;
  float x_top = t.position[sorted[0]].x;

  float y_mid = t.position[sorted[1]].y;
  float x_mid = t.position[sorted[1]].x;

  float y_bot = t.position[sorted[2]].y;
  float x_bot = t.position[sorted[2]].x;

  // std::cout << "y_top = " << y_top << std::endl;
  // std::cout << "x_top = " << y_top << std::endl;

  // std::cout << "y_mid = " << y_mid << std::endl;
  // std::cout << "x_mid = " << y_mid << std::endl;

  // std::cout << "y_bot = " << y_bot << std::endl;
  // std::cout << "x_bot = " << y_bot << std::endl;

  float y = y_top;
  float xl = x_top;
  float xr = x_top;

  float delta_tm = (x_mid - x_top) / (y_mid - y_top);
  float delta_tb = (x_bot - x_top) / (y_bot - y_top);
  float delta_mb = (x_bot - x_mid) / (y_bot - y_mid);

  // std::cout << "delta_tm = " << delta_tm << std::endl;
  // std::cout << "delta_tb = " << delta_tb << std::endl;
  // std::cout << "delta_mb = " << delta_mb << std::endl;
  
  vec3 color_top = t.color[sorted[0]];
  vec3 color_mid = t.color[sorted[1]];
  vec3 color_bot = t.color[sorted[2]];


  if (delta_tm < delta_tb) { // case 1: xl += delta_tm

    do
    {
      SimpleRasterizer::DrawSpan(xl, xr, y, 0, 0, color_top, color_top);
      y++;
      if (y >= y_mid) // case: middle point is reached, change of slope 
        xl += delta_mb;
      else 
        xl += delta_tm;
      xr += delta_tb;

    } while (y < y_bot);
    

  } else { // case 2: xl += delta_tb
    do
    {
      
      SimpleRasterizer::DrawSpan(xl, xr, y, 0, 0, color_top, color_top);
      y++;
      if (y >= y_mid) // case: middle point is reached, change of slope
        xr += delta_mb;
      else 
        xr += delta_tm;
      xl += delta_tb;
    } while (y < y_bot);
  }

}

vec3 SimpleRasterizer::LightVertex(vec4 position, vec3 normal, vec3 color)
{
  vec3 result = color * ambientLight;

  foreach (const Light *, light, lights)
  {
    vec3 intensity = vec3(1.0f);
    if ((*light)->IsInstanceOf(SceneObjectType_PointLight))
      intensity = ((PointLight *)*light)->GetIntensity();

    vec3 distance = (*light)->GetPosition() - vec3(position);
    float attenuation = 1.0f / (0.001f + dot(distance, distance));
    vec3 direction = normalize(distance);

    float lambert = glm::max(0.0f, dot(normal, direction));

    if (lambert > 0)
      result += color * lambert * attenuation * intensity;
  }

  return result;
}


void SimpleRasterizer::SortTriangles(vector<Triangle> &triangles)
{
  sort(triangles.begin(), triangles.end(), CompareTriangle);
}


void SimpleRasterizer::TransformAndLightTriangle(Triangle &t, 
                                                 const mat4 &modelTransform,
                                                 const mat4 &modelTransformNormals)
{ 
  // Exercise 8.1 c)
  
  for (int i=0; i<3; i++) {
    // Apply model transform to go from model coordiantes to world coordinates
    vec4 worldCoords =  modelTransform * vec4(t.position[i], 1.0f);
    vec3 worldNormal = normalize(vec3(modelTransformNormals * vec4(t.normal[i], 0.0f)));

    // Light vertex in world coordinates
    t.color[i] = LightVertex(worldCoords, worldNormal, t.color[i]);

    // Get clip coordinates by ViewProjectionTransformation
    vec4 clipCoords = this->viewProjectionTransform * worldCoords;

    // Get normalized device coordinates (i.e. x,y in [-1,1])
    clipCoords.x /= clipCoords.w;
    clipCoords.y /= clipCoords.w;
    clipCoords.z /= clipCoords.w;
    
    // Apply viewport transform to get windows coordinates and set new positions
    t.position[i].x = (clipCoords.x +1.0)*(image->GetWidth()/2.0);
    t.position[i].y = (-1.0f*clipCoords.y +1.0)*(image->GetHeight()/2.0);
  }
}


void SimpleRasterizer::RenderMesh(const Mesh *mesh)
{
  if (mesh == NULL)
    return;

  // Exercise 8.1 a)

  const mat4 modelTransform = mesh->GetGlobalTransformation();
  const mat4 modelTransformNormals = inverseTranspose(modelTransform);

  for (Triangle t: mesh->GetTriangles()) {
    this->TransformAndLightTriangle(t, modelTransform, modelTransformNormals);
    this->DrawTriangle(t);
  }
}

void SimpleRasterizer::ScanObject(const Raytracer::Scenes::SceneObject *object)
{
  if (object == NULL)
    return;

  if (object->IsInstanceOf(SceneObjectType_Light))
    lights.push_back((const Light *)object);
  if (object->IsInstanceOf(SceneObjectType_Mesh))
    meshes.push_back((const Mesh *)object);

  foreach_c(SceneObject *, child, object->GetChildren())
    ScanObject(*child);
}

bool SimpleRasterizer::Render(Image &image, const Scene &scene)
{
  image.Clear(vec3(0));

  Camera *camera = scene.GetActiveCamera();
  if (camera == NULL)
    return false;

  zBuffer = new float[image.GetWidth() * image.GetHeight()];
  for (int i = 0; i < image.GetWidth() * image.GetHeight(); i++)
    zBuffer[i] = 1.0f;

  // Build lists of all lights and meshes in the scene.
  lights.clear();
  meshes.clear();
  ScanObject(&scene);

  //Exercise 8.1 b)

  const mat4 viewTransformation = lookAt(
    camera->GetEye(),    // The position of your camera, in world space
    camera->GetLookAt(), // where you want to look at, in world space
    camera->GetUp()      // probably glm::vec3(0,1,0), but (0,-1,0) would make you looking upside-down, which can be great too
  );

  // Generates a really hard-to-read matrix, but a normal, standard 4x4 matrix nonetheless
  const mat4 projectionMatrix = perspective(
    camera->GetFov(),      // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
    camera->GetAspect(),   // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
    camera->GetNearClip(), // Near clipping plane. Keep as big as possible, or you'll get precision issues.
    camera->GetFarClip()   // Far clipping plane. Keep as little as possible.
  );

  this->viewProjectionTransform = projectionMatrix * viewTransformation;
  
  // Render all meshes we found.
  this->image = &image;
  foreach(const Mesh *, mesh, meshes)
    RenderMesh(*mesh);

  delete[] zBuffer;

  return true;
}
