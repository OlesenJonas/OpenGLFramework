/*
    http://www.cs.otago.ac.nz/postgrads/alexis/planeExtraction.pdf
    Extracts the frustum planes from:
        a projection matrix -> result planes are in view space
        a projection * view matrix -> results are in world space
        ...
    Normal of planes points towards "inside" halfspace
*/
vec4[6] extractFrustumPlanes(const mat4 projectionMatrix)
{
    return vec4[6](
        vec4( //left
        projectionMatrix[0][3] + projectionMatrix[0][0],
        projectionMatrix[1][3] + projectionMatrix[1][0],
        projectionMatrix[2][3] + projectionMatrix[2][0],
        projectionMatrix[3][3] + projectionMatrix[3][0]
        ),
        vec4( //right
        projectionMatrix[0][3] - projectionMatrix[0][0],
        projectionMatrix[1][3] - projectionMatrix[1][0],
        projectionMatrix[2][3] - projectionMatrix[2][0],
        projectionMatrix[3][3] - projectionMatrix[3][0]
        ),
        vec4( //bottom
        projectionMatrix[0][3] + projectionMatrix[0][1],
        projectionMatrix[1][3] + projectionMatrix[1][1],
        projectionMatrix[2][3] + projectionMatrix[2][1],
        projectionMatrix[3][3] + projectionMatrix[3][1]
        ),
        vec4( //top
        projectionMatrix[0][3] - projectionMatrix[0][1],
        projectionMatrix[1][3] - projectionMatrix[1][1],
        projectionMatrix[2][3] - projectionMatrix[2][1],
        projectionMatrix[3][3] - projectionMatrix[3][1]
        ),
        vec4( //near
        projectionMatrix[0][3] + projectionMatrix[0][2],
        projectionMatrix[1][3] + projectionMatrix[1][2],
        projectionMatrix[2][3] + projectionMatrix[2][2],
        projectionMatrix[3][3] + projectionMatrix[3][2]
        ),
        vec4( //far
        projectionMatrix[0][3] - projectionMatrix[0][2],
        projectionMatrix[1][3] - projectionMatrix[1][2],
        projectionMatrix[2][3] - projectionMatrix[2][2],
        projectionMatrix[3][3] - projectionMatrix[3][2]
        )
    );
}

/*
    triangleCorners[i].w needs to be 1
*/
bool triangleInsideFrustum(inout vec4[3] triangleCorners, const vec4[6] frustaPlanes)
{
    bool inside = false;
    for(int i=0; i<6; i++)
    {
        // inside = inside || (0.0 < triangleCorners[0].x * frustaPlanes[i].x + triangleCorners[0].y * frustaPlanes[i].y + triangleCorners[0].z * frustaPlanes[i].z + frustaPlanes[i].w);
        // inside = inside || (0.0 < triangleCorners[1].x * frustaPlanes[i].x + triangleCorners[1].y * frustaPlanes[i].y + triangleCorners[1].z * frustaPlanes[i].z + frustaPlanes[i].w);
        // inside = inside || (0.0 < triangleCorners[2].x * frustaPlanes[i].x + triangleCorners[2].y * frustaPlanes[i].y + triangleCorners[2].z * frustaPlanes[i].z + frustaPlanes[i].w);
        inside = inside || (0.0 < dot(triangleCorners[0],frustaPlanes[i]));
        inside = inside || (0.0 < dot(triangleCorners[1],frustaPlanes[i]));
        inside = inside || (0.0 < dot(triangleCorners[2],frustaPlanes[i]));
        if(!inside)
            return false;
        inside = false;
    }
    return true;
}

float getTriangleMaxEdgeLengthInPixels(vec4[3] triangleCorners, const mat4 projectionMatrix, const vec2 screenRes)
{
    //transform to screen space
    triangleCorners[0] = projectionMatrix * triangleCorners[0];
    triangleCorners[1] = projectionMatrix * triangleCorners[1];
    triangleCorners[2] = projectionMatrix * triangleCorners[2];
    triangleCorners[0] /= triangleCorners[0].w;
    triangleCorners[1] /= triangleCorners[1].w;
    triangleCorners[2] /= triangleCorners[2].w;
    triangleCorners[0].xy = 0.5*triangleCorners[0].xy+0.5;
    triangleCorners[1].xy = 0.5*triangleCorners[1].xy+0.5;
    triangleCorners[2].xy = 0.5*triangleCorners[2].xy+0.5;
    triangleCorners[0].xy *= screenRes;
    triangleCorners[1].xy *= screenRes;
    triangleCorners[2].xy *= screenRes;

    float edge0 = distance(triangleCorners[0].xy, triangleCorners[1].xy);
    float edge1 = distance(triangleCorners[1].xy, triangleCorners[2].xy);
    float edge2 = distance(triangleCorners[2].xy, triangleCorners[0].xy);

    return max(edge0,max(edge1,edge2));
}