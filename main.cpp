#include "pr.hpp"
#include <iostream>
#include <memory>

glm::vec3 sampleCosWeighted(float a, float b) {
    float s = std::sqrt(a);
    return glm::vec3(
        s * std::cos(glm::two_pi<float>() * b),
        s * std::sin(glm::two_pi<float>() * b),
        std::sqrt(glm::max(1.0f - a, 0.0f))
    );
}

int main() {
    using namespace pr;

    Config config;
    config.ScreenWidth = 1920;
    config.ScreenHeight = 1080;
    config.SwapInterval = 1;
    Initialize(config);

    struct SamplePoint
    {
        glm::vec3 p;
    };
    SamplePoint points[64];

    Xoshiro128StarStar random;
    int N = 8;
    for (int j = 0; j < N; ++j)
	{
        for (int i = 0; i < N; ++i)
		{
			float u = glm::mix( (float)j / N, (float)(j + 1) / N, random.uniformf());
			float v = glm::mix( (float)i / N, (float)(i + 1) / N, random.uniformf());
			points[j * N + i].p = sampleCosWeighted(u, v);
		}
	}
    
    const float r = 1.0f;
    const int RES = 128;
    struct Occlusions
    {
        uint64_t data[RES][RES][RES];
    };
    Occlusions* occlusions = new Occlusions();
    
    LinearTransform i2v(0, RES - 1, -1.0f, 1.0f);
    ParallelFor(RES, [&](int iz) {
        for (int iy = 0; iy < RES; ++iy)
        {
            for (int ix = 0; ix < RES; ++ix)
            {
                float x = i2v(ix);
                float y = i2v(iy);
                float z = i2v(iz);
                glm::vec3 P = glm::vec3(x, y, z);
                float p_length = glm::length(P);
                glm::vec3 np = P / p_length;
                float d = (1.0f - p_length) * 8.0f / 7.0f * r;

                uint64_t o = 0;
                for( int i = 0; i < 64; ++i )
                {
                    if (d < glm::dot(points[i].p, np))
                    {
                        o |= (1LLU << i );
                    }
                }
                occlusions->data[iz][iy][ix] = o;

                //LinearTransform v2i(-1.0f, 1.0f, 0, RES - 1);

                //// d = 0 then
                //float s = 1.0f - (7.0f / 8.0f) * d / r;
                //int ix_r = glm::clamp((int)round(v2i(np.x * s)), 0, RES - 1);
                //int iy_r = glm::clamp((int)round(v2i(np.y * s)), 0, RES - 1);
                //int iz_r = glm::clamp((int)round(v2i(np.z * s)), 0, RES - 1);

                //printf("%d, %d, %d\n", ix - ix_r, iy - iy_r, iz - iz_r);
            }
        }
    });
    //for (int iz = 0; iz < RES; ++iz)
    //{
    //}

    Camera3D camera;
    camera.origin = { 4, 4, 4 };
    camera.lookat = { 0, 0, 0 };
    camera.zUp = true;

    double e = GetElapsedTime();

    while (pr::NextFrame() == false) {
        if (IsImGuiUsingMouse() == false) 
        {
            UpdateCameraBlenderLike(&camera);
        }

        ClearBackground(0.1f, 0.1f, 0.1f, 1);

        BeginCamera(camera);

        PushGraphicState();

        DrawGrid(GridAxis::XY, 1.0f, 10, { 128, 128, 128 });
        DrawXYZAxis(1.0f);

        // manual visual
        static float d = 0.0f;
        static glm::vec3 D = glm::vec3(0.866461754, -0.499243587, 0);
        //ManipulatePosition(camera, &D, 0.2f);
        //glm::vec3 np = glm::normalize(D);
        //float dist = 0.0f;
        //DrawArrow(np * d, np* d + np, 0.02f, { 255, 255, 0 });
       
        //{
        //    LinearTransform v2i(-1.0f, 1.0f, 0, RES - 1);

        //    // d = 0 then

        //    float s = 1.0f - (7.0f / 8.0f) * d / r;
        //    int ix = glm::clamp((int)round(v2i(np.x * s)), 0, RES - 1);
        //    int iy = glm::clamp((int)round(v2i(np.y * s)), 0, RES - 1);
        //    int iz = glm::clamp((int)round(v2i(np.z * s)), 0, RES - 1);

        //    uint64_t o = occlusions->data[iz][iy][ix];
        //    for (int i = 0; i < 64; ++i)
        //    {
        //        // char label[128];
        //        // sprintf(label, "%d", i);

        //        if (o & (1LLU << i))
        //        {
        //            DrawPoint(points[i].p, { 255, 64, 64 }, 2);
        //            // DrawText(points[i].p + glm::vec3(0, 0, 0.1f), label, 8.0f, { 255, 0, 0 });
        //        }
        //        else
        //        {
        //            DrawPoint(points[i].p, { 64, 64, 64 }, 2);
        //            // DrawText(points[i].p + glm::vec3(0, 0, 0.1f), label, 8.0f);
        //        }
        //    }
        //}

        static Xoshiro128StarStar random(50);
        static glm::vec3 vertices[3] = {
            {1.0f, -1.0f, 0.7f},
            {0.0f, 1.0f, 0.9f},
            {-1.0f, -1.0f, 0.7f},
        };


        glm::vec3 triNg = glm::normalize( glm::cross(vertices[1] - vertices[0], vertices[2] - vertices[1]) );
        glm::vec3 triO = (vertices[0] + vertices[1] + vertices[2]) / 3.0f;
        DrawArrow(triO, triO + triNg * 0.2f, 0.01f, { 255, 0, 255 });

        // scalar triple product determine flip or not
        float triple = glm::dot(glm::cross(vertices[0], vertices[1]), vertices[2]);

        for (int i = 0; i < 3; ++i)
        {
            DrawLine(vertices[i], vertices[(i + 1) % 3], { 255, 255, 255 });
            ManipulatePosition(camera, &vertices[i], 0.2f);

            char label[128];
            sprintf(label, "%d", i);
            DrawText(vertices[i], label);

            LinearTransform k2s(0, 32, 0, 1);
            for (int k = 0; k < 32; ++k)
            {
                glm::vec3 p0 = glm::normalize( glm::mix(vertices[i], vertices[(i + 1) % 3], k2s(k)) );
                glm::vec3 p1 = glm::normalize( glm::mix(vertices[i], vertices[(i + 1) % 3], k2s(k + 1)));
                DrawLine(p0, p1, { 0, 255, 255 });
            }
        }
        {
            uint64_t mask = std::numeric_limits<uint64_t>::max();
            LinearTransform v2i(-1.0f, 1.0f, 0, RES - 1);
            for (int i = 0; i < 3; ++i )
            {
                glm::vec3 p0 = vertices[i];
                glm::vec3 p1 = vertices[(i + 1) % 3];
                glm::vec3 np = glm::normalize( glm::cross(p0, p1) );

                float s = 1.0f - (7.0f / 8.0f) * (0.0f /* d */) / r;
                int ix = glm::clamp((int)round(v2i(np.x * s)), 0, RES - 1);
                int iy = glm::clamp((int)round(v2i(np.y * s)), 0, RES - 1);
                int iz = glm::clamp((int)round(v2i(np.z * s)), 0, RES - 1);
                uint64_t o = occlusions->data[iz][iy][ix];

                // !!flip case!!
                if( triple < 0.0f )
                {
                    o = ~o;
                }
                mask &= o;
            }

            // be carefull flip case!!
            float dTri = glm::dot(triNg, vertices[0]);
            DrawArrow({0, 0, 0}, triNg * dTri, 0.01f, { 255, 0, 255 });
            uint64_t triangleMask;
            {
                
                float sign = triple < 0.0f ? -1.0f : 1.0f;

                float s = 1.0f - (7.0f / 8.0f) * dTri * sign / r;
                int ix = glm::clamp((int)round(v2i(triNg.x * s * sign)), 0, RES - 1);
                int iy = glm::clamp((int)round(v2i(triNg.y * s * sign)), 0, RES - 1);
                int iz = glm::clamp((int)round(v2i(triNg.z * s * sign)), 0, RES - 1);

                triangleMask = occlusions->data[iz][iy][ix];
            }

            // tri
            //for (int i = 0; i < 64; ++i)
            //{
            //    if (triangleMask & (1LLU << i))
            //    {
            //        DrawPoint(points[i].p, { 255, 64, 64 }, 4);
            //    }
            //    else
            //    {
            //        DrawPoint(points[i].p, { 64, 64, 255 }, 2);
            //    }
            //}

            for (int i = 0; i < 64; ++i)
            {
                if (mask & (1LLU << i))
                {
                    if( mask & triangleMask & (1LLU << i) )
                    {
                        DrawPoint(points[i].p, { 255, 64, 64 }, 4);
                    }
                    else
                    {
                        DrawPoint(points[i].p, { 64, 64, 255 }, 2);
                    }
                    // DrawText(points[i].p + glm::vec3(0, 0, 0.1f), label, 8.0f, { 255, 0, 0 });
                }
                else
                {
                    DrawPoint(points[i].p, { 128, 128, 128 }, 2);
                    // DrawText(points[i].p + glm::vec3(0, 0, 0.1f), label, 8.0f);
                }
            }
        }

        //{
        //    float x = i2v(29);
        //    float y = i2v(8);
        //    float z = i2v(16);
        //    glm::vec3 P = glm::vec3(x, y, z);
        //    float p_length = glm::length(P);
        //    glm::vec3 np = P / p_length;
        //    float d = (1.0f - p_length) * 8.0f / 7.0f * r;

        //    uint64_t o = 0;
        //    for (int i = 0; i < 64; ++i)
        //    {
        //        if (glm::dot(points[i].p, np) < d)
        //        {
        //            DrawPoint(points[i].p, { 255, 64, 64 }, 2);
        //        }
        //        else
        //        {
        //            DrawPoint(points[i].p, { 64, 64, 64 }, 2);
        //        }
        //    }
        //    occlusions->data[iz][iy][ix] = o;

        //    DrawArrow(glm::vec3(0.0f), np, 0.02f, { 255, 0, 255 });
        //}

        //int RES = 128;
        //LinearTransform i2v(0, RES, 0.f, 1.0f);
        //for (int iz = 0; iz < RES; ++iz)
        //{
        //    for (int iphi = 0; iphi < RES; ++iphi)
        //    {
        //        float z = i2v(iz);
        //        float phi = i2v(iphi) * glm::two_pi<float>();

        //        float xy = sqrt(1.0f - z * z);
        //        glm::vec3 d = {
        //            xy * glm::cos( phi ),
        //            xy * glm::sin( phi ),
        //            z
        //        };
        //        DrawPoint(d, { 128, 128, 128 }, 2);
        //    }
        //}

        //static float K = 7.0f / 8.0f;
        //float r = 1.0f;
        //int RES = 32;
        //LinearTransform i2v(0, RES, -1.0f, 1.0f);
        //for (int iz = 0; iz < RES; ++iz)
        //{
        //    for (int iy = 0; iy < RES; ++iy)
        //    {
        //        for (int ix = 0; ix < RES; ++ix)
        //        {
        //            float x = i2v(ix);
        //            float y = i2v(iy);
        //            float z = i2v(iz);
        //            glm::vec3 P = glm::vec3(x, y, z);
        //            float p_length = glm::length(P);
        //            glm::vec3 np = P / p_length;
        //            // float d = (1.0f - p_length) * 8.0f / 7.0f * r;
        //            float d = (1.0f - p_length) / K * r;
        //            DrawPoint(np * d, { 128, 128, 128 }, 2);
        //        }
        //    }
        //}

        PopGraphicState();
        EndCamera();

        BeginImGui();

        ImGui::SetNextWindowSize({ 500, 800 }, ImGuiCond_Once);
        ImGui::Begin("Panel");
        ImGui::Text("fps = %f", GetFrameRate());
        ImGui::TextColored(ImColor(255, 64, 64), "Red : Occluded ");
        ImGui::TextColored(ImColor(64, 64, 255), "Blue: Not Occluded by Tri Plane ");
        ImGui::TextColored(ImColor(128, 128, 128), "Gray: Not Occluded by Tri Side ");
        ImGui::Text("Red: Occlude ");
        ImGui::SliderFloat("d", &d, 0, 1);
        ImGui::Text("triple = %f", triple);
        
        // ImGui::SliderFloat("7/8", &K, 0, 1);

        ImGui::End();

        EndImGui();
    }

    pr::CleanUp();
}
