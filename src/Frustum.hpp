#pragma once
#include <cmath>
#include <glm/glm.hpp>

struct Frustum {
    glm::vec4 p[6];
};

inline Frustum extractFrustum(const glm::mat4& M)
{
    glm::vec4 r0{ M[0][0], M[1][0], M[2][0], M[3][0] };
    glm::vec4 r1{ M[0][1], M[1][1], M[2][1], M[3][1] };
    glm::vec4 r2{ M[0][2], M[1][2], M[2][2], M[3][2] };
    glm::vec4 r3{ M[0][3], M[1][3], M[2][3], M[3][3] };

    auto norm = [](glm::vec4 v) {
        float l = glm::length(glm::vec3(v));
        return v / l;
    };

    Frustum f;
    f.p[0] = norm(r3 + r0);
    f.p[1] = norm(r3 - r0);
    f.p[2] = norm(r3 + r1);
    f.p[3] = norm(r3 - r1);
    f.p[4] = norm(r3 + r2);
    f.p[5] = norm(r3 - r2);
    return f;
}

inline bool aabbInFrustum(const Frustum& f, const glm::vec3& mn, const glm::vec3& mx)
{
    for (int i = 0; i < 6; ++i) {
        glm::vec3 n = glm::vec3(f.p[i]);
        glm::vec3 p = { n.x > 0 ? mx.x : mn.x, n.y > 0 ? mx.y : mn.y, n.z > 0 ? mx.z : mn.z };
        if (glm::dot(n, p) + f.p[i].w < 0)
            return false;
    }
    return true;
}
