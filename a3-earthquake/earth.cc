/** CSci-4611 Assignment 3:  Earthquake
 */

#include "earth.h"
#include "config.h"

#include <vector>

// for M_PI constant
#define _USE_MATH_DEFINES
#include <math.h>


Earth::Earth() {
    scale = 0.018;
    totalTimeR = 0.0;
    totalTimeS = 0.0;
}

Earth::~Earth() {
}

void Earth::Init(const std::vector<std::string> &search_path) {
    // init shader program
    shader_.Init();
    
    // init texture: you can change to a lower-res texture here if needed
    earth_tex_.InitFromFile(Platform::FindFile("earth-2k.png", search_path));

    // init geometry


    // TODO: This is where you need to set the vertices and indiceds for earth_mesh_.

    // As a demo, we'll add a square with 2 triangles.
    std::vector<unsigned int> indices;
    
    std::vector<Point2> tex_coords;

    /*
    // four vertices
    vertices.push_back(Point3(0,0,0));
    vertices.push_back(Point3(1,0,0));
    vertices.push_back(Point3(1,1,0));
    vertices.push_back(Point3(0,1,0));

    // indices into the arrays above for the first triangle
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    
    // indices for the second triangle, note some are reused
    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(3);
    */

    float ii = 2.0 * GfxMath::ToDegrees(M_PI) / nslices;

    float jj = 2.0 * GfxMath::ToDegrees(M_PI_2) / nstacks;

    float u = 0.0;
    for (float i = -GfxMath::ToDegrees(M_PI); i <= GfxMath::ToDegrees(M_PI); i += ii) {
        float v = 1.0;
        for (float j = -GfxMath::ToDegrees(M_PI_2); j <= GfxMath::ToDegrees(M_PI_2); j += jj) {
            vertices.push_back(LatLongToPlane(i,j));
            tex_coords.push_back(Point2(u, v));
            normals.push_back(Vector3(0, 0, 1).ToUnit());
            v -= 1.0 / nstacks;
        }
        if (i != -GfxMath::ToDegrees(M_PI)) {
            int last_vert_index = vertices.size() - 1;
            for (int stacks = 0; stacks <= nstacks - 1; stacks++) {
                indices.push_back(last_vert_index);
                indices.push_back(last_vert_index - (nstacks + 1));
                indices.push_back(last_vert_index - 1);

                indices.push_back(last_vert_index - 1);
                indices.push_back(last_vert_index - (nstacks + 1));
                indices.push_back(last_vert_index - (nstacks + 2));
                last_vert_index--;
            }
        }
        u += 1.0 / nslices;
    }




    for (int i = 0; i < vertices.size(); i++) {
        float lat = vertices[i][1];
        float lon = vertices[i][0];
        Vector3 nor = LatLongToSphere(lat, lon) - Point3(0, 0, 0);
        verticesS.push_back(LatLongToSphere(lat, lon));
        normalsS.push_back(nor.ToUnit());
    }

    earth_mesh_.SetTexCoords(0, tex_coords);
    earth_mesh_.SetVertices(vertices);
    earth_mesh_.SetIndices(indices);
    earth_mesh_.SetNormals(normals);
    earth_mesh_.UpdateGPUMemory();
}



void Earth::Draw(const Matrix4 &model_matrix, const Matrix4 &view_matrix, const Matrix4 &proj_matrix) {
    // Define a really bright white light.  Lighting is a property of the "shader"
    DefaultShader::LightProperties light;
    light.position = Point3(10,10,10);
    light.ambient_intensity = Color(1,1,1);
    light.diffuse_intensity = Color(1,1,1);
    light.specular_intensity = Color(1,1,1);
    shader_.SetLight(0, light);

    // Adust the material properties, material is a property of the thing
    // (e.g., a mesh) that we draw with the shader.  The reflectance properties
    // affect the lighting.  The surface texture is the key for getting the
    // image of the earth to show up.
    DefaultShader::MaterialProperties mat;
    mat.ambient_reflectance = Color(0.5, 0.5, 0.5);
    mat.diffuse_reflectance = Color(0.75, 0.75, 0.75);
    mat.specular_reflectance = Color(0.75, 0.75, 0.75);
    mat.surface_texture = earth_tex_;

    // Draw the earth mesh using these settings
    if (earth_mesh_.num_triangles() > 0) {
        shader_.Draw(model_matrix, view_matrix, proj_matrix, &earth_mesh_, mat);
    }
}


Point3 Earth::LatLongToSphere(double latitude, double longitude) const {
    // TODO: We recommend filling in this function to put all your
    // lat,long --> sphere calculations in one place.
    float x = cos(latitude) * sin(longitude);
    float y = sin(latitude);
    float z = cos(latitude) * cos(longitude);
    return Point3(x,y,z);
}

Point3 Earth::LatLongToPlane(double latitude, double longitude) const {
    // TODO: We recommend filling in this function to put all your
    // lat,long --> plane calculations in one place.
    return Point3(scale *latitude, scale *longitude,0);
}



void Earth::DrawDebugInfo(const Matrix4 &model_matrix, const Matrix4 &view_matrix, const Matrix4 &proj_matrix) {
    // This draws a cylinder for each line segment on each edge of each triangle in your mesh.
    // So it will be very slow if you have a large mesh, but it's quite useful when you are
    // debugging your mesh code, especially if you start with a small mesh.
    for (int t=0; t<earth_mesh_.num_triangles(); t++) {
        std::vector<unsigned int> indices = earth_mesh_.triangle_verticesReadOnly(t);
        std::vector<Point3> loop;
        loop.push_back(earth_mesh_.vertexReadOnly(indices[0]));
        loop.push_back(earth_mesh_.vertexReadOnly(indices[1]));
        loop.push_back(earth_mesh_.vertexReadOnly(indices[2]));
        quick_shapes_.DrawLines(model_matrix, view_matrix, proj_matrix,
            Color(1,1,0), loop, QuickShapes::LinesType::LINE_LOOP, 0.005);
    }
}

std::vector<float> normalizeList(std::vector<float> quakeList) {
    float maxMagnitude = *std::max_element(quakeList.begin(), quakeList.end());
    std::vector<float> result;
    for (int i = 0; i < quakeList.size(); i++)
        result.push_back(quakeList[i] / maxMagnitude);
    return result;
}


void Earth::globeMode(bool isPressed) {
    /*
    vertices.clear();
    normals.clear();

    float ii = 2.0 * M_PI / nslices;
    float jj = 2.0 * M_PI_2 / nstacks;
    float r = 1;

    for (float i = -M_PI; i <= M_PI; i += ii) {
        float x = r * cos(i);
        float z = r * -sin(i);
        for (float j = -M_PI_2; j <= M_PI_2; j += jj) {

            vertices.push_back(Point3(x, j, z));
            normals.push_back(Vector3(x, 0, z).ToUnit());
            
            if (j < 0) {

                r += 0.1;
            }else{
                r -= 0.1;
            }
            
        }
       
    }
    */
    if (isPressed == false) {
        earth_mesh_.SetVertices(vertices);
        earth_mesh_.SetNormals(normals);
        earth_mesh_.UpdateGPUMemory();

    }
    else {
        earth_mesh_.SetVertices(verticesS);
        earth_mesh_.SetNormals(normalsS);
        earth_mesh_.UpdateGPUMemory();

    }
}

void Earth::morphRtoS(double dt) {
    totalTimeS = 0.0;
    totalTimeR += dt;
    std::vector<Point3> verticesL;
    if (totalTimeR <= 1.0) {
            for (int i = 0; i < vertices.size(); i++) {
                verticesL.push_back(vertices[i].Lerp(vertices[i], verticesS[i], totalTimeR));
            } 
        earth_mesh_.SetVertices(verticesL);
        earth_mesh_.UpdateGPUMemory();

    }
}

void Earth::morphStoR(double dt) {
    totalTimeR = 0.0;
    totalTimeS += dt;
    std::vector<Point3> verticesM;
    if (totalTimeS <= 1.0) {

        for (int i = 0; i < vertices.size(); i++) {
            verticesM.push_back(verticesS[i].Lerp(verticesS[i], vertices[i], totalTimeS));
        }

        earth_mesh_.SetVertices(verticesM);
        earth_mesh_.UpdateGPUMemory();
    }
}


/*
std::vector<Point3> Earth::morphRtoS(double dt, std::vector<Point3> rPos, std::vector<Point3> sPos) {
    totalTimeS = 0.0;
    totalTimeR += dt;
    std::vector<Point3> verticesL;
    std::vector<Point3> ret;
    if (totalTimeR <= 1.0) {
        for (int i = 0; i < vertices.size(); i++) {
            verticesL.push_back(vertices[i].Lerp(vertices[i], verticesS[i], totalTimeR));
        }
        earth_mesh_.SetVertices(verticesL);
        earth_mesh_.UpdateGPUMemory();
        for (int j = 0; j < rPos.size(); j++) {
            ret.push_back(rPos[j].Lerp(rPos[j], sPos[j], totalTimeR));
        }
    }
    return ret;
}

std::vector<Point3> Earth::morphStoR(double dt, std::vector<Point3> rPos, std::vector<Point3> sPos) {
    totalTimeR = 0.0;
    totalTimeS += dt;
    std::vector<Point3> ret;
    std::vector<Point3> verticesM;
    if (totalTimeS <= 1.0) {

        for (int i = 0; i < vertices.size(); i++) {
            verticesM.push_back(verticesS[i].Lerp(verticesS[i], vertices[i], totalTimeS));
        }

        earth_mesh_.SetVertices(verticesM);
        earth_mesh_.UpdateGPUMemory();
        for (int j = 0; j < rPos.size(); j++) {
            ret.push_back(sPos[j].Lerp(sPos[j], rPos[j], totalTimeR));
        }
    }
    return ret;
}
*/