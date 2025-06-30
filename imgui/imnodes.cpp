// the structure of this file:
//
// [SECTION] bezier curve helpers
// [SECTION] draw list helper
// [SECTION] ui state logic
// [SECTION] render helpers
// [SECTION] API implementation

#include "imnodes_internal.h"

// Check minimum ImGui version
#define MINIMUM_COMPATIBLE_IMGUI_VERSION 17400
#if IMGUI_VERSION_NUM < MINIMUM_COMPATIBLE_IMGUI_VERSION
#error "Minimum ImGui version requirement not met -- please use a newer version!"
#endif

#include <limits.h>
#include <math.h>
#include <new>
#include <stdint.h>
#include <stdio.h> // for fwrite, ssprintf, sscanf
#include <stdlib.h>
#include <string.h> // strlen, strncmp

// Use secure CRT function variants to avoid MSVC compiler errors
#ifdef _MSC_VER
#define sscanf sscanf_s
#endif

ImNodesContext* GImNodes = NULL;

namespace IMNODES_NAMESPACE
{
namespace
{
// [SECTION] bezier curve helpers

struct CubicBezier
{
    ImVec2 P0, P1, P2, P3;
    int    NumSegments;
};

inline ImVec2 EvalCubicBezier(
    const float   t,
    const ImVec2& P0,
    const ImVec2& P1,
    const ImVec2& P2,
    const ImVec2& P3)
{
    // B(t) = (1-t)**3 p0 + 3(1 - t)**2 t P1 + 3(1-t)t**2 P2 + t**3 P3

    const float u = 1.0f - t;
    const float b0 = u * u * u;
    const float b1 = 3 * u * u * t;
    const float b2 = 3 * u * t * t;
    const float b3 = t * t * t;
    return ImVec2(
        b0 * P0.x + b1 * P1.x + b2 * P2.x + b3 * P3.x,
        b0 * P0.y + b1 * P1.y + b2 * P2.y + b3 * P3.y);
}

// Calculates the closest point along each bezier curve segment.
ImVec2 GetClosestPointOnCubicBezier(const int num_segments, const ImVec2& p, const CubicBezier& cb)
{
    IM_ASSERT(num_segments > 0);
    ImVec2 p_last = cb.P0;
    ImVec2 p_closest;
    float  p_closest_dist = FLT_MAX;
    float  t_step = 1.0f / (float)num_segments;
    for (int i = 1; i <= num_segments; ++i)
    {
        ImVec2 p_current = EvalCubicBezier(t_step * i, cb.P0, cb.P1, cb.P2, cb.P3);
        ImVec2 p_line = ImLineClosestPoint(p_last, p_current, p);
        float  dist = ImLengthSqr(p - p_line);
        if (dist < p_closest_dist)
        {
            p_closest = p_line;
            p_closest_dist = dist;
        }
        p_last = p_current;
    }
    return p_closest;
}

inline float GetDistanceToCubicBezier(
    const ImVec2&      pos,
    const CubicBezier& cubic_bezier,
    const int          num_segments)
{
    const ImVec2 point_on_curve = GetClosestPointOnCubicBezier(num_segments, pos, cubic_bezier);

    const ImVec2 to_curve = point_on_curve - pos;
    return ImSqrt(ImLengthSqr(to_curve));
}

inline ImRect GetContainingRectForCubicBezier(const CubicBezier& cb)
{
    const ImVec2 min = ImVec2(ImMin(cb.P0.x, cb.P3.x), ImMin(cb.P0.y, cb.P3.y));
    const ImVec2 max = ImVec2(ImMax(cb.P0.x, cb.P3.x), ImMax(cb.P0.y, cb.P3.y));

    const float hover_distance = GImNodes->Style.LinkHoverDistance;

    ImRect rect(min, max);
    rect.Add(cb.P1);
    rect.Add(cb.P2);
    rect.Expand(ImVec2(hover_distance, hover_distance));

    return rect;
}

inline CubicBezier GetCubicBezier(
    ImVec2                          start,
    ImVec2                          end,
    const ImNodesAttributeType      start_type,
    const float                     line_segments_per_length,
    const ImNodesLinkDeformations   deformation)
{
    IM_ASSERT(
        (start_type == ImNodesAttributeType_Input) || (start_type == ImNodesAttributeType_Output));
    if (start_type == ImNodesAttributeType_Input)
    {
        ImSwap(start, end);
    }

    const float  link_length = ImSqrt(ImLengthSqr(end - start));
    const ImVec2 offset = ImVec2(0.25f * link_length, 0.f);
    CubicBezier  cubic_bezier;
    cubic_bezier.P0 = start + deformation[0];
    cubic_bezier.P1 = start + offset + deformation[1];
    cubic_bezier.P2 = end - offset + deformation[4];
    cubic_bezier.P3 = end + deformation[5];
    cubic_bezier.NumSegments = ImMax(static_cast<int>(link_length * line_segments_per_length), 1);
    return cubic_bezier;
}

inline CubicBezier GetCubicBezier(
    ImVec2                          start,
    ImVec2                          end,
    const ImNodesAttributeType      start_type,
    const ImNodesAttributeType      end_type,
    const float                     line_segments_per_length,
    const ImNodesLinkDeformations   deformation)
{
    IM_ASSERT(
        (start_type == ImNodesAttributeType_Input) || (start_type == ImNodesAttributeType_Output));
    /*if (start_type == ImNodesAttributeType_Input)
    {
        ImSwap(start, end);
    }*/

    const float  link_length = ImSqrt(ImLengthSqr(end - start));
    const ImVec2 offset = ImVec2(0.25f * link_length, 0.f);
    CubicBezier  cubic_bezier;
    cubic_bezier.P0 = start + deformation[0];
    if(start_type == ImNodesAttributeType_Output)
        cubic_bezier.P1 = start + offset + deformation[1];
    else
        cubic_bezier.P1 = start - offset + deformation[1];
    if (end_type == ImNodesAttributeType_Input)
        cubic_bezier.P2 = end - offset + deformation[4];
    else
        cubic_bezier.P2 = end + offset + deformation[4];
    cubic_bezier.P3 = end + deformation[5];
    cubic_bezier.NumSegments = ImMax(static_cast<int>(link_length * line_segments_per_length), 1);
    return cubic_bezier;
}

// [SECTION] sloped curve helpers

// a slope curve is a line made of 3 segment, the first and the last segment are horizontal and of minimal length 'LinkSlopedMinOffset'. The middle segment is a sloped segment of minimal slope 'LinkSlopedMinSlope'
//If the slope is suppose to go backward, it add 2 poitn to make a loop like form
struct SlopedCurve
{
    ImVec2 P[6];
    int NumSegments;
};

//inline ImVec2 EvalSlopedCurve(
//    const float   t,
//    const ImVec2& P0,
//    const ImVec2& P1,
//    const ImVec2& P2,
//    const ImVec2& P3)
//{
//    // B(t) = (1-t)**3 p0 + 3(1 - t)**2 t P1 + 3(1-t)t**2 P2 + t**3 P3
//
//    const float u = 1.0f - t;
//    const float b0 = u * u * u;
//    const float b1 = 3 * u * u * t;
//    const float b2 = 3 * u * t * t;
//    const float b3 = t * t * t;
//    return ImVec2(
//        b0 * P0.x + b1 * P1.x + b2 * P2.x + b3 * P3.x,
//        b0 * P0.y + b1 * P1.y + b2 * P2.y + b3 * P3.y);
//}

// Calculates the closest point along each segment.
ImVec2 GetClosestPointOnSlopedCurve(const int num_segments, const ImVec2& p, const SlopedCurve& sc)
{
    //ImVec2 p_last = sc.P0;
    ImVec2 p_last = sc.P[0];
    ImVec2 p_closest;
    float  p_closest_dist = FLT_MAX;
    for (int i = 1; i <= num_segments+1; ++i)
    {
        //ImVec2 p_current = EvalCubicBezier(t_step * i, cb.P0, cb.P1, cb.P2, cb.P3);
        ImVec2 p_current = sc.P[i];
        //ImVec2 p_current = *(& (sc.P1) + i); //dirty c hack //MIGHT NEED A CHECK
        ImVec2 p_line = ImLineClosestPoint(p_last, p_current, p);
        float  dist = ImLengthSqr(p - p_line);
        if (dist < p_closest_dist)
        {
            p_closest = p_line;
            p_closest_dist = dist;
        }
        p_last = p_current;
    }
    return p_closest;
}

inline float GetDistanceToSlopedCurve(
    const ImVec2& pos,
    const SlopedCurve& sloped_curve,
    const int          num_segments)
{
    const ImVec2 point_on_curve = GetClosestPointOnSlopedCurve(num_segments, pos, sloped_curve);

    const ImVec2 to_curve = point_on_curve - pos;
    return ImSqrt(ImLengthSqr(to_curve));
}

inline ImRect GetContainingRectForSlopedCurve(const SlopedCurve& sc)
{
    /*const ImVec2 min = ImVec2(ImMin(sc.P[0].x, sc.P[3].x), ImMin(sc.P[0].y, sc.P[3].y));
    const ImVec2 max = ImVec2(ImMax(sc.P[0].x, sc.P[3].x), ImMax(sc.P[0].y, sc.P[3].y));

    const float hover_distance = GImNodes->Style.LinkHoverDistance;

    ImRect rect(min, max);
    rect.Add(sc.P[1]);
    rect.Add(sc.P[2]);
    rect.Expand(ImVec2(hover_distance, hover_distance));*/

    const float hover_distance = GImNodes->Style.LinkHoverDistance;

    ImRect rect(ImVec2(FLT_MAX, FLT_MAX), ImVec2(-FLT_MAX, -FLT_MAX));
    for (int i = 0; i < sc.NumSegments+1; i++)
        rect.Add(sc.P[i]);
    rect.Expand(ImVec2(hover_distance, hover_distance));

    return rect;
}

inline SlopedCurve GetSlopedCurve(
    ImVec2                          start,
    ImVec2                          end,
    const ImNodesAttributeType      start_type,
    const ImNodesLinkDeformations   deformation)
{
    IM_ASSERT(
        (start_type == ImNodesAttributeType_Input) || (start_type == ImNodesAttributeType_Output));
    if (start_type == ImNodesAttributeType_Input)
    {
        ImSwap(start, end);
    }
    //start is either an output or the mouse

    float min_slope = ImNodes::GetStyle().LinkSlopedMinSlope;
    float min_offset = ImNodes::GetStyle().LinkSlopedMinOffset;

    ImVec2 offset = ImVec2(min_offset, 0.0f);
    SlopedCurve sloped_curve;
    sloped_curve.P[0] = start + deformation[0];
    sloped_curve.P[1] = start + offset + deformation[1];

    if (end.x >= start.x + 2*min_offset) {
        //standart sloped curve
        float h = end.y - start.y;
        float d = abs(h) / min_slope;//distance traveld with the min slope
        if (min_slope > 0.001f && d < end.x - start.x - 2 * min_offset)
            sloped_curve.P[2] = start + offset + ImVec2(d, h) + deformation[4];
        else
            sloped_curve.P[2] = end - offset + deformation[4];
        sloped_curve.P[3] = end + deformation[5];
        sloped_curve.NumSegments = 3;
    }
    else {
        float mid_y = (end.y + start.y) / 2;
        sloped_curve.P[2] = ImVec2(start.x + min_offset, mid_y) + deformation[2];
        sloped_curve.P[3] = ImVec2(end.x - min_offset, mid_y) + deformation[3];
        sloped_curve.P[4] = end - offset + deformation[4];
        sloped_curve.P[5] = end + deformation[5];
        sloped_curve.NumSegments = 5;
    }
    return sloped_curve;
}

inline SlopedCurve GetSlopedCurve(
    ImVec2                          start,
    ImVec2                          end,
    const ImNodesAttributeType      start_type,
    const ImNodesAttributeType      end_type,
    const ImNodesLinkDeformations   deformation)
{
    return GetSlopedCurve(start, end, start_type, deformation);
}

inline float EvalImplicitLineEq(const ImVec2& p1, const ImVec2& p2, const ImVec2& p)
{
    return (p2.y - p1.y) * p.x + (p1.x - p2.x) * p.y + (p2.x * p1.y - p1.x * p2.y);
}

inline int Sign(float val) { return int(val > 0.0f) - int(val < 0.0f); }

inline bool RectangleOverlapsLineSegment(const ImRect& rect, const ImVec2& p1, const ImVec2& p2)
{
    // Trivial case: rectangle contains an endpoint
    if (rect.Contains(p1) || rect.Contains(p2))
    {
        return true;
    }

    // Flip rectangle if necessary
    ImRect flip_rect = rect;

    if (flip_rect.Min.x > flip_rect.Max.x)
    {
        ImSwap(flip_rect.Min.x, flip_rect.Max.x);
    }

    if (flip_rect.Min.y > flip_rect.Max.y)
    {
        ImSwap(flip_rect.Min.y, flip_rect.Max.y);
    }

    // Trivial case: line segment lies to one particular side of rectangle
    if ((p1.x < flip_rect.Min.x && p2.x < flip_rect.Min.x) ||
        (p1.x > flip_rect.Max.x && p2.x > flip_rect.Max.x) ||
        (p1.y < flip_rect.Min.y && p2.y < flip_rect.Min.y) ||
        (p1.y > flip_rect.Max.y && p2.y > flip_rect.Max.y))
    {
        return false;
    }

    const int corner_signs[4] = {
        Sign(EvalImplicitLineEq(p1, p2, flip_rect.Min)),
        Sign(EvalImplicitLineEq(p1, p2, ImVec2(flip_rect.Max.x, flip_rect.Min.y))),
        Sign(EvalImplicitLineEq(p1, p2, ImVec2(flip_rect.Min.x, flip_rect.Max.y))),
        Sign(EvalImplicitLineEq(p1, p2, flip_rect.Max))};

    int sum = 0;
    int sum_abs = 0;

    for (int i = 0; i < 4; ++i)
    {
        sum += corner_signs[i];
        sum_abs += abs(corner_signs[i]);
    }

    // At least one corner of rectangle lies on a different side of line segment
    return abs(sum) != sum_abs;
}

inline bool RectangleOverlapsBezier(const ImRect& rectangle, const CubicBezier& cubic_bezier)
{
    ImRect bounding_rect = ImRect(cubic_bezier.P0, cubic_bezier.P0);
    bounding_rect.Add(cubic_bezier.P1);
    bounding_rect.Add(cubic_bezier.P2);
    bounding_rect.Add(cubic_bezier.P3);
    if (!rectangle.Overlaps(bounding_rect))
        return false;
    if(rectangle.Contains(cubic_bezier.P0) || rectangle.Contains(cubic_bezier.P3))
        return true;

    ImVec2 current =
        EvalCubicBezier(0.f, cubic_bezier.P0, cubic_bezier.P1, cubic_bezier.P2, cubic_bezier.P3);
    const float dt = 1.0f / cubic_bezier.NumSegments;
    for (int s = 0; s < cubic_bezier.NumSegments; ++s)
    {
        ImVec2 next = EvalCubicBezier(
            static_cast<float>((s + 1) * dt),
            cubic_bezier.P0,
            cubic_bezier.P1,
            cubic_bezier.P2,
            cubic_bezier.P3);
        if (RectangleOverlapsLineSegment(rectangle, current, next))
        {
            return true;
        }
        current = next;
    }
    return false;
}

inline bool RectangleOverlapsSlopedCurve(const ImRect& rectangle, const SlopedCurve& sloped_curve)
{
    ImRect bounding_rect = ImRect(sloped_curve.P[0], sloped_curve.P[0]);
    bounding_rect.Add(sloped_curve.P[1]);
    bounding_rect.Add(sloped_curve.P[2]);
    bounding_rect.Add(sloped_curve.P[3]);
    if (sloped_curve.NumSegments == 5) {
        bounding_rect.Add(sloped_curve.P[4]);
        bounding_rect.Add(sloped_curve.P[5]);
    }

    if (!rectangle.Overlaps(bounding_rect))
        return false;


    for (int i = 0; i < sloped_curve.NumSegments; ++i)
    {
        if (RectangleOverlapsLineSegment(rectangle, sloped_curve.P[i], sloped_curve.P[i+1]))
        {
            return true;
        }
    }
    return false;
}

struct Curve {
    CubicBezier* cubic_bezier;
    SlopedCurve* sloped_curve;
    ImNodesLinkType type;
};

ImVec2 GetClosestPointOnCurve(const int num_segments, const ImVec2& p, const Curve& c)
{
    switch (c.type)
    {
    case ImNodesLinkType_::ImNodesLinkType_Bezier:
        return GetClosestPointOnCubicBezier(num_segments, p, *c.cubic_bezier);
    case ImNodesLinkType_::ImNodesLinkType_Sloped:
        return GetClosestPointOnSlopedCurve(num_segments, p, *c.sloped_curve);
    }
}

inline float GetDistanceToCurve(
    const ImVec2& pos,
    const Curve& curve,
    const int num_segments)
{
    const ImVec2 point_on_curve = GetClosestPointOnCurve(num_segments, pos, curve);

    const ImVec2 to_curve = point_on_curve - pos;
    return ImSqrt(ImLengthSqr(to_curve));
}

inline ImRect GetContainingRectForCurve(const Curve& c)
{
    switch (c.type)
    {
    case ImNodesLinkType_::ImNodesLinkType_Bezier:
        return GetContainingRectForCubicBezier(*c.cubic_bezier);
    case ImNodesLinkType_::ImNodesLinkType_Sloped:
        return GetContainingRectForSlopedCurve(*c.sloped_curve);
    }
}

inline Curve GetCurve(
    ImVec2                          start,
    ImVec2                          end,
    const ImNodesAttributeType      start_type,
    const float                     line_segments_per_length,
    const ImNodesLinkType           type,
    const ImNodesLinkDeformations   deformation)
{
    Curve curve{NULL, NULL, type};
    switch (curve.type)
    {
    case ImNodesLinkType_::ImNodesLinkType_Bezier:
        curve.cubic_bezier = new CubicBezier(GetCubicBezier(start, end, start_type, line_segments_per_length, deformation));
        break;
    case ImNodesLinkType_::ImNodesLinkType_Sloped:
        curve.sloped_curve = new SlopedCurve(GetSlopedCurve(start, end, start_type, deformation));
        break;
    }
    return curve;
}

inline Curve GetCurve(
    ImVec2                          start,
    ImVec2                          end,
    const ImNodesAttributeType      start_type,
    const ImNodesAttributeType      end_type,
    const float                     line_segments_per_length,
    const ImNodesLinkType           type,
    const ImNodesLinkDeformations   deformation)
{
    Curve curve{ NULL, NULL, type };
    switch (curve.type)
    {
    case ImNodesLinkType_::ImNodesLinkType_Bezier:
        curve.cubic_bezier = new CubicBezier(GetCubicBezier(start, end, start_type, end_type, line_segments_per_length, deformation));
        break;
    case ImNodesLinkType_::ImNodesLinkType_Sloped:
        curve.sloped_curve = new SlopedCurve(GetSlopedCurve(start, end, start_type, end_type, deformation));
        break;
    }
    return curve;
}

void DrawCubicBezier(const CubicBezier cubic_bezier, ImU32 col, float thick) {
#if IMGUI_VERSION_NUM < 18000
    GImNodes->CanvasDrawList->AddBezierCurve(
#else
    GImNodes->CanvasDrawList->AddBezierCubic(
#endif
        cubic_bezier.P0,
        cubic_bezier.P1,
        cubic_bezier.P2,
        cubic_bezier.P3,
        col,
        thick,
        cubic_bezier.NumSegments);
}

void DrawSlopedCurve(const SlopedCurve sloped_curve, ImU32 col, float thick) {
    GImNodes->CanvasDrawList->AddPolyline(sloped_curve.P, sloped_curve.NumSegments+1, col, 0, thick);
}

void DrawCurve(const Curve curve, ImU32 col, float thick) {
    switch (curve.type)
    {
    case ImNodesLinkType_::ImNodesLinkType_Bezier:
        return DrawCubicBezier(*curve.cubic_bezier, col, thick);
    case ImNodesLinkType_::ImNodesLinkType_Sloped:
        return DrawSlopedCurve(*curve.sloped_curve, col, thick);
    }
}

int CurveNumSegments(const Curve curve) {
    switch (curve.type)
    {
    case ImNodesLinkType_::ImNodesLinkType_Bezier:
        return curve.cubic_bezier->NumSegments;
    case ImNodesLinkType_::ImNodesLinkType_Sloped:
        return curve.sloped_curve->NumSegments;
    }
}

inline bool RectangleOverlapsCurve(const ImRect& rectangle, const Curve& curve)
{
    switch (curve.type)
    {
    case ImNodesLinkType_::ImNodesLinkType_Bezier:
        return RectangleOverlapsBezier(rectangle, *curve.cubic_bezier);
    case ImNodesLinkType_::ImNodesLinkType_Sloped:
        return RectangleOverlapsSlopedCurve(rectangle, *curve.sloped_curve);
    }
}

inline bool RectangleOverlapsLink(
    const ImRect&                   rectangle,
    const ImVec2&                   start,
    const ImVec2&                   end,
    const ImNodesAttributeType      start_type,
    const ImNodesLinkType           linkType,
    const ImNodesLinkDeformations   deformation)
{    
    const Curve curve = GetCurve(start, end, start_type, GImNodes->Style.LinkLineSegmentsPerLength, linkType, deformation);
    return RectangleOverlapsCurve(rectangle, curve);
}

struct ControlPrimitive {
    ImVec2 P0;
    ImVec2 P1;
    ImNodesLinkControlType Type;
};

ControlPrimitive GetControlPrimitiveBezier(
    int                Id, 
    const CubicBezier& cb) {
    IM_ASSERT(Id == 0 || Id == 1 || Id == 4 || Id == 5);
    ControlPrimitive cp;
    cp.Type = ImNodesLinkControlType_Point;
    switch (Id)
    {
    case 0:
        cp.P0 = cb.P0;
        break;
    case 1:
        cp.P0 = cb.P1;
        break;
    case 4:
        cp.P0 = cb.P2;
        break;
    case 5:
        cp.P0 = cb.P3;
        break;
    }
    return cp;
}

int GetControlPrimitivePointsLocalID(int localID, const SlopedCurve& curve, int localID_dest[2]) {
    IM_ASSERT(localID >= 0 && localID < 11);
    ControlPrimitive cp;
    if (curve.NumSegments == 3) {
        IM_ASSERT((localID != 2) && (localID != 3) && (localID != 7) && (localID != 9)); //localID used for curve with 5 segment
        if (localID < 6) {// point
            //localID_dest[0] = localID < 2 ? localID : localID - 2;
            localID_dest[0] = localID; //localId is not changing, is the index in the list of point link.P that is changing
            return 1;
        }
        else {// segment
            if (localID == 8) {
                localID_dest[0] = 1;
                localID_dest[1] = 4;
                return 2;
            }
            localID_dest[0] = localID / 2 - 3;
            localID_dest[1] = localID_dest[0] + 1;
            return 2;
        }
    }
    else
    {
        if (localID < 6) {//point
            localID_dest[0] = localID;
            return 1;
        }
        else
        {
            localID_dest[0] = localID - 6;
            localID_dest[1] = localID_dest[0] + 1;
            return 2;
        }
    }
}

ControlPrimitive GetControlPrimitiveSlopedCurve(
    int                Id, 
    const SlopedCurve& curve) {
    //TODO Rewrite it using GetControlPrimitivePointsLocalID
    IM_ASSERT(Id >= 0 && Id < 11);
    ControlPrimitive cp;
    if (curve.NumSegments == 3) {
        IM_ASSERT((Id != 2) && (Id != 3) && (Id != 7) && (Id != 9)); //Id used for curve with 5 segment
        if (Id < 6) {// point
            cp.Type = ImNodesLinkControlType_Point;
            cp.P0 = curve.P[Id < 2 ? Id : Id - 2];
        }
        else {// segment
            cp.Type = ImNodesLinkControlType_Segment;
            Id = Id / 2 - 3; //convert [6, 8, 10] into [0, 1, 2]
            cp.P0 = curve.P[Id];
            cp.P1 = curve.P[Id+1];
        }
    }
    else 
    {
        if (Id < 6) {//point
            cp.Type = ImNodesLinkControlType_Point;
            cp.P0 = curve.P[Id];
        }
        else 
        {
            cp.Type = ImNodesLinkControlType_Segment;
            Id = Id - 6; //convert[6, 7, 8, 9, 10] -> [0, 1, 2, 3, 4]
            cp.P0 = curve.P[Id];
            cp.P1 = curve.P[Id + 1];
        }
    }
    return cp;    
}

const ControlPrimitive GetControlPrimitive(
    int          Id,
    const Curve& curve) {
    switch (curve.type)
    {
    case ImNodesLinkType_::ImNodesLinkType_Bezier:
        return GetControlPrimitiveBezier(Id, *curve.cubic_bezier);
    case ImNodesLinkType_::ImNodesLinkType_Sloped:
        return GetControlPrimitiveSlopedCurve(Id, *curve.sloped_curve);
    default:
        IM_ASSERT(!"Unreachable code!");
        break;
    }
}

const float GetDistanceToControlPrimitive(
    const ImVec2&           pos,
    const ControlPrimitive& cp) {
    ImVec2 closest_point;

    switch (cp.Type) {
    case ImNodesLinkControlType_Point:
        closest_point = cp.P0;
        break;
    case ImNodesLinkControlType_Segment:
        closest_point = ImLineClosestPoint(cp.P0, cp.P1, pos);
        break;
    default:
        IM_ASSERT(!"Unreachable code!");
        break;
    }

    const ImVec2 to_curve = closest_point - pos;
    return ImSqrt(ImLengthSqr(to_curve));
}

void DrawControlPrimitive(const ControlPrimitive& cp, ImU32 col, float thick) {
    
    switch (cp.Type) {
    case ImNodesLinkControlType_Point:
        GImNodes->CanvasDrawList->AddCircleFilled(cp.P0, thick*1.5f, col);
        break;
    case ImNodesLinkControlType_Segment:
        GImNodes->CanvasDrawList->AddLine(cp.P0, cp.P1, col, thick);
        break;
    default:
        IM_ASSERT(!"Unreachable code!");
        break;
    }
}

inline bool RectangleOverlapsControlPrimitive(const ImRect& rectangle, const ControlPrimitive& cp) {
    if (cp.Type == ImNodesLinkControlType_Segment) {
        return RectangleOverlapsLineSegment(rectangle, cp.P0, cp.P1);
    }
    else {
        return rectangle.Contains(cp.P0);
    }
}

ImVec2 GetLinkControlOrigin(const ImNodesEditorContext& editor, const ImLinkControlData& linkControl) {
    const ImLinkData link = editor.Links.Pool[linkControl.LinkIdx];
    const ImPinData start_pin = editor.Pins.Pool[link.StartPinIdx];
    const ImPinData end_pin = editor.Pins.Pool[link.EndPinIdx];
    const Curve curve = GetCurve(
        start_pin.Pos, end_pin.Pos, start_pin.Type, end_pin.Type, GImNodes->Style.LinkLineSegmentsPerLength, link.LinkType, link.Deformations);
    ImVec2 origin = ImVec2(0, 0);
    if (linkControl.LocalId == 1 || linkControl.LocalId == 4 || linkControl.LocalId == 2 || linkControl.LocalId == 3) {
        if (curve.type == ImNodesLinkType_Sloped) {
            if (curve.sloped_curve->NumSegments == 3)
                origin = curve.sloped_curve->P[linkControl.LocalId < 2 ? linkControl.LocalId : linkControl.LocalId - 2];
            else
                origin = curve.sloped_curve->P[linkControl.LocalId];
        }
    }
    else if (linkControl.LocalId == 7 || linkControl.LocalId == 8 || linkControl.LocalId == 9) {
        if (curve.sloped_curve->NumSegments == 3)
            origin = curve.sloped_curve->P[linkControl.LocalId - 6];//can only be locaIl 8
        else
            origin = curve.sloped_curve->P[linkControl.LocalId-6+1];
    }
    return origin;
}

ImVec2 MooveLinkControl(ImNodesEditorContext& editor, const ImLinkControlData& linkControl, ImVec2 origin) {
    ImLinkData& link = editor.Links.Pool[linkControl.LinkIdx];
    const ImPinData start_pin = editor.Pins.Pool[link.StartPinIdx];
    const ImPinData end_pin = editor.Pins.Pool[link.EndPinIdx];
    const Curve curve = GetCurve(
        start_pin.Pos, end_pin.Pos, start_pin.Type, end_pin.Type, GImNodes->Style.LinkLineSegmentsPerLength, link.LinkType, link.Deformations);
    ImVec2 linkOrigin = GetLinkControlOrigin(editor, linkControl);
    if (linkControl.LocalId == 1 || linkControl.LocalId == 4 || linkControl.LocalId == 2 || linkControl.LocalId == 3) {
        if (curve.type == ImNodesLinkType_Sloped) {
            link.Deformations[linkControl.LocalId] += origin - linkOrigin;
        }
    }
    else if (linkControl.LocalId == 7 || linkControl.LocalId == 8 || linkControl.LocalId == 9) {
        if (curve.type == ImNodesLinkType_Sloped) {
            if (curve.sloped_curve->NumSegments != 3) {
                link.Deformations[linkControl.LocalId - 6] += origin - linkOrigin;
                link.Deformations[linkControl.LocalId - 6 + 1] += origin - linkOrigin;
            }
            else if (linkControl.LocalId == 8) {
                link.Deformations[1] += origin - linkOrigin;
                link.Deformations[4] += origin - linkOrigin;
            }
        }
    }
    return ImVec2(0, 0);
}
static void PushEventVar(const ImNodesEventVarElement& Event);
void ContraintLinkControl(ImNodesEditorContext& editor, const ImLinkControlData& linkControl) {
    ImLinkData& link = editor.Links.Pool[linkControl.LinkIdx];
    const ImPinData start_pin = editor.Pins.Pool[link.StartPinIdx];
    const ImPinData end_pin = editor.Pins.Pool[link.EndPinIdx];
    const Curve curve = GetCurve(
        start_pin.Pos, end_pin.Pos, start_pin.Type, end_pin.Type, GImNodes->Style.LinkLineSegmentsPerLength, link.LinkType, link.Deformations);

    ImNodesEventVarElement Event = ImNodesEventVarElement(ImNodesEventVar_LinkDeformation);
    int localID_dest[2];
    int nb_points = GetControlPrimitivePointsLocalID(linkControl.LocalId, *curve.sloped_curve, localID_dest);
    for (int i = 0; i < nb_points; i++)
        Event.addOldPos(GetLinkControlId(localID_dest[i], link.Id), link.Deformations[localID_dest[i]]);

    if (linkControl.LocalId == 1 || linkControl.LocalId == 6) {
        link.Deformations[1].y = 0;

        if (start_pin.Type == ImNodesAttributeType_Output && link.Deformations[1].x < 0)
            link.Deformations[1].x = 0;
        if (start_pin.Type == ImNodesAttributeType_Input && link.Deformations[1].x > 0)
            link.Deformations[1].x = 0;
    }
    if (linkControl.LocalId == 4 || linkControl.LocalId == 10) {
        link.Deformations[4].y = 0;
        if (end_pin.Type == ImNodesAttributeType_Output && link.Deformations[4].x < 0)
            link.Deformations[4].x = 0;
        if (end_pin.Type == ImNodesAttributeType_Input && link.Deformations[4].x > 0)
            link.Deformations[4].x = 0;
    }
    if (linkControl.LocalId == 2) {
        link.Deformations[2].y = link.Deformations[3].y;
        link.Deformations[2].x = link.Deformations[1].x;
    }
    if (linkControl.LocalId == 3) {
        link.Deformations[3].y = link.Deformations[2].y;
        link.Deformations[3].x = link.Deformations[4].x;
    }

    if (linkControl.LocalId == 7) {
        float avg_x = (link.Deformations[1].x + link.Deformations[2].x)/2.0f;
        link.Deformations[1].x = avg_x;
        link.Deformations[2].x = avg_x;
    }
    if (linkControl.LocalId == 8) {
        if (curve.sloped_curve->NumSegments == 3) {
            float avg_x = (link.Deformations[1].x + link.Deformations[4].x) / 2.0f;
            float min_slope = ImNodes::GetStyle().LinkSlopedMinSlope;
            if (min_slope > 0.001f) {
                float h = (link.Deformations[1].y - link.Deformations[4].y);
                float x = h/min_slope;
                link.Deformations[1].x = avg_x + x/2;
                link.Deformations[4].x = avg_x - x/2;
            }
            else {
                link.Deformations[1].x = avg_x;
                link.Deformations[4].x = avg_x;
            }
        }
        else {
            float avg_y = (link.Deformations[2].y + link.Deformations[3].y) / 2.0f;
            link.Deformations[2].y = avg_y;
            link.Deformations[3].y = avg_y;
        }
        
    }
    if (linkControl.LocalId == 9) {
        float avg_x = (link.Deformations[3].x + link.Deformations[4].x) / 2.0f;
        link.Deformations[3].x = avg_x;
        link.Deformations[4].x = avg_x;
    }
    for (int i = 0; i < nb_points; i++)
        Event.addNewPos(GetLinkControlId(localID_dest[i], link.Id), link.Deformations[localID_dest[i]]);
    if (Event.valid())
        PushEventVar(Event);
    editor.current_event = ImNodesEventVarElement();
}


void ContraintLabel(ImNodesEditorContext& editor, ImLabelData& Label) {
    ImNodesEventVarElement Event = ImNodesEventVarElement(ImNodesEventVar_LabelMove);
    Event.addOldPos(Label.Id, Label.Deformation);
    Label.Deformation = ImVec2(0, 0);
    Event.addNewPos(Label.Id, Label.Deformation);

    if (Event.valid())
        PushEventVar(Event);
    editor.current_event = ImNodesEventVarElement();
}

// [SECTION] coordinate space conversion helpers

inline ImVec2 ScreenSpaceToGridSpace(const ImNodesEditorContext& editor, const ImVec2& v)
{
    return v - GImNodes->CanvasOriginScreenSpace - editor.Panning;
}

inline ImRect ScreenSpaceToGridSpace(const ImNodesEditorContext& editor, const ImRect& r)
{
    return ImRect(ScreenSpaceToGridSpace(editor, r.Min), ScreenSpaceToGridSpace(editor, r.Max));
}

inline ImVec2 GridSpaceToScreenSpace(const ImNodesEditorContext& editor, const ImVec2& v)
{
    return v + GImNodes->CanvasOriginScreenSpace + editor.Panning;
}

inline ImVec2 GridSpaceToEditorSpace(const ImNodesEditorContext& editor, const ImVec2& v)
{
    return v + editor.Panning;
}

inline ImVec2 EditorSpaceToGridSpace(const ImNodesEditorContext& editor, const ImVec2& v)
{
    return v - editor.Panning;
}

inline ImVec2 EditorSpaceToScreenSpace(const ImVec2& v)
{
    return GImNodes->CanvasOriginScreenSpace + v;
}

inline ImVec2 MiniMapSpaceToGridSpace(const ImNodesEditorContext& editor, const ImVec2& v)
{
    return (v - editor.MiniMapContentScreenSpace.Min) / editor.MiniMapScaling +
           editor.GridContentBounds.Min;
}

inline ImVec2 ScreenSpaceToMiniMapSpace(const ImNodesEditorContext& editor, const ImVec2& v)
{
    return (ScreenSpaceToGridSpace(editor, v) - editor.GridContentBounds.Min) *
               editor.MiniMapScaling +
           editor.MiniMapContentScreenSpace.Min;
}

inline ImRect ScreenSpaceToMiniMapSpace(const ImNodesEditorContext& editor, const ImRect& r)
{
    return ImRect(
        ScreenSpaceToMiniMapSpace(editor, r.Min), ScreenSpaceToMiniMapSpace(editor, r.Max));
}

// [SECTION] draw list helper

void ImDrawListGrowChannels(ImDrawList* draw_list, const int num_channels)
{
    ImDrawListSplitter& splitter = draw_list->_Splitter;

    if (splitter._Count == 1)
    {
        splitter.Split(draw_list, num_channels + 1);
        return;
    }

    // NOTE: this logic has been lifted from ImDrawListSplitter::Split with slight modifications
    // to allow nested splits. The main modification is that we only create new ImDrawChannel
    // instances after splitter._Count, instead of over the whole splitter._Channels array like
    // the regular ImDrawListSplitter::Split method does.

    const int old_channel_capacity = splitter._Channels.Size;
    // NOTE: _Channels is not resized down, and therefore _Count <= _Channels.size()!
    const int old_channel_count = splitter._Count;
    const int requested_channel_count = old_channel_count + num_channels;
    if (old_channel_capacity < old_channel_count + num_channels)
    {
        splitter._Channels.resize(requested_channel_count);
    }

    splitter._Count = requested_channel_count;

    for (int i = old_channel_count; i < requested_channel_count; ++i)
    {
        ImDrawChannel& channel = splitter._Channels[i];

        // If we're inside the old capacity region of the array, we need to reuse the existing
        // memory of the command and index buffers.
        if (i < old_channel_capacity)
        {
            channel._CmdBuffer.resize(0);
            channel._IdxBuffer.resize(0);
        }
        // Else, we need to construct new draw channels.
        else
        {
            IM_PLACEMENT_NEW(&channel) ImDrawChannel();
        }

        {
            ImDrawCmd draw_cmd;
            draw_cmd.ClipRect = draw_list->_ClipRectStack.back();
            draw_cmd.TextureId = draw_list->_TextureIdStack.back();
            channel._CmdBuffer.push_back(draw_cmd);
        }
    }
}

void ImDrawListSplitterSwapChannels(
    ImDrawListSplitter& splitter,
    const int           lhs_idx,
    const int           rhs_idx)
{
    if (lhs_idx == rhs_idx)
    {
        return;
    }

    IM_ASSERT(lhs_idx >= 0 && lhs_idx < splitter._Count);
    IM_ASSERT(rhs_idx >= 0 && rhs_idx < splitter._Count);

    ImDrawChannel& lhs_channel = splitter._Channels[lhs_idx];
    ImDrawChannel& rhs_channel = splitter._Channels[rhs_idx];
    lhs_channel._CmdBuffer.swap(rhs_channel._CmdBuffer);
    lhs_channel._IdxBuffer.swap(rhs_channel._IdxBuffer);

    const int current_channel = splitter._Current;

    if (current_channel == lhs_idx)
    {
        splitter._Current = rhs_idx;
    }
    else if (current_channel == rhs_idx)
    {
        splitter._Current = lhs_idx;
    }
}

void DrawListSet(ImDrawList* window_draw_list)
{
    GImNodes->CanvasDrawList = window_draw_list;
    GImNodes->NodeIdxToSubmissionIdx.Clear();
    GImNodes->NodeIdxSubmissionOrder.clear();
}

// The draw list channels are structured as follows. First we have our base channel, the canvas grid
// on which we render the grid lines in BeginNodeEditor(). The base channel is the reason
// draw_list_submission_idx_to_background_channel_idx offsets the index by one. Each BeginNode()
// call appends two new draw channels, for the node background and foreground. The node foreground
// is the channel into which the node's ImGui content is rendered. Finally, in EndNodeEditor() we
// append one last draw channel for rendering the selection box and the incomplete link on top of
// everything else.
//
// +----------+----------+----------+----------+----------+----------+
// |          |          |          |          |          |          |
// |canvas    |node      |node      |...       |...       |click     |
// |grid      |background|foreground|          |          |interaction
// |          |          |          |          |          |          |
// +----------+----------+----------+----------+----------+----------+
//            |                     |
//            |   submission idx    |
//            |                     |
//            -----------------------

void DrawListAddNode(const int node_idx)
{
    GImNodes->NodeIdxToSubmissionIdx.SetInt(
        static_cast<ImGuiID>(node_idx), GImNodes->NodeIdxSubmissionOrder.Size);
    GImNodes->NodeIdxSubmissionOrder.push_back(node_idx);
    ImDrawListGrowChannels(GImNodes->CanvasDrawList, 2);
}

void DrawListAppendClickInteractionChannel()
{
    // NOTE: don't use this function outside of EndNodeEditor. Using this before all nodes have been
    // added will screw up the node draw order.
    ImDrawListGrowChannels(GImNodes->CanvasDrawList, 1);
}

int DrawListSubmissionIdxToBackgroundChannelIdx(const int submission_idx)
{
    // NOTE: the first channel is the canvas background, i.e. the grid
    return 1 + 2 * submission_idx;
}

int DrawListSubmissionIdxToForegroundChannelIdx(const int submission_idx)
{
    return DrawListSubmissionIdxToBackgroundChannelIdx(submission_idx) + 1;
}

void DrawListActivateClickInteractionChannel()
{
    GImNodes->CanvasDrawList->_Splitter.SetCurrentChannel(
        GImNodes->CanvasDrawList, GImNodes->CanvasDrawList->_Splitter._Count - 1);
}

void DrawListActivateCurrentNodeForeground()
{
    const int foreground_channel_idx =
        DrawListSubmissionIdxToForegroundChannelIdx(GImNodes->NodeIdxSubmissionOrder.Size - 1);
    GImNodes->CanvasDrawList->_Splitter.SetCurrentChannel(
        GImNodes->CanvasDrawList, foreground_channel_idx);
}

void DrawListActivateNodeBackground(const int node_idx)
{
    const int submission_idx =
        GImNodes->NodeIdxToSubmissionIdx.GetInt(static_cast<ImGuiID>(node_idx), -1);
    // There is a discrepancy in the submitted node count and the rendered node count! Did you call
    // one of the following functions
    // * EditorContextMoveToNode
    // * SetNodeScreenSpacePos
    // * SetNodeGridSpacePos
    // * SetNodeDraggable
    // after the BeginNode/EndNode function calls?
    IM_ASSERT(submission_idx != -1);
    const int background_channel_idx = DrawListSubmissionIdxToBackgroundChannelIdx(submission_idx);
    GImNodes->CanvasDrawList->_Splitter.SetCurrentChannel(
        GImNodes->CanvasDrawList, background_channel_idx);
}

void DrawListSwapSubmissionIndices(const int lhs_idx, const int rhs_idx)
{
    IM_ASSERT(lhs_idx != rhs_idx);

    const int lhs_foreground_channel_idx = DrawListSubmissionIdxToForegroundChannelIdx(lhs_idx);
    const int lhs_background_channel_idx = DrawListSubmissionIdxToBackgroundChannelIdx(lhs_idx);
    const int rhs_foreground_channel_idx = DrawListSubmissionIdxToForegroundChannelIdx(rhs_idx);
    const int rhs_background_channel_idx = DrawListSubmissionIdxToBackgroundChannelIdx(rhs_idx);

    ImDrawListSplitterSwapChannels(
        GImNodes->CanvasDrawList->_Splitter,
        lhs_background_channel_idx,
        rhs_background_channel_idx);
    ImDrawListSplitterSwapChannels(
        GImNodes->CanvasDrawList->_Splitter,
        lhs_foreground_channel_idx,
        rhs_foreground_channel_idx);
}

void DrawListSortChannelsByDepth(const ImVector<int>& node_idx_depth_order)
{
    if (GImNodes->NodeIdxToSubmissionIdx.Data.Size < 2)
    {
        return;
    }

    IM_ASSERT(node_idx_depth_order.Size == GImNodes->NodeIdxSubmissionOrder.Size);

    int start_idx = node_idx_depth_order.Size - 1;

    while (node_idx_depth_order[start_idx] == GImNodes->NodeIdxSubmissionOrder[start_idx])
    {
        if (--start_idx == 0)
        {
            // early out if submission order and depth order are the same
            return;
        }
    }

    // TODO: this is an O(N^2) algorithm. It might be worthwhile revisiting this to see if the time
    // complexity can be reduced.

    for (int depth_idx = start_idx; depth_idx > 0; --depth_idx)
    {
        const int node_idx = node_idx_depth_order[depth_idx];

        // Find the current index of the node_idx in the submission order array
        int submission_idx = -1;
        for (int i = 0; i < GImNodes->NodeIdxSubmissionOrder.Size; ++i)
        {
            if (GImNodes->NodeIdxSubmissionOrder[i] == node_idx)
            {
                submission_idx = i;
                break;
            }
        }
        IM_ASSERT(submission_idx >= 0);

        if (submission_idx == depth_idx)
        {
            continue;
        }

        for (int j = submission_idx; j < depth_idx; ++j)
        {
            DrawListSwapSubmissionIndices(j, j + 1);
            ImSwap(GImNodes->NodeIdxSubmissionOrder[j], GImNodes->NodeIdxSubmissionOrder[j + 1]);
        }
    }
}

// [SECTION] ui state logic

ImVec2 GetScreenSpacePinCoordinates(
    const ImRect&              node_rect,
    const ImRect&              attribute_rect,
    const ImNodesAttributeType type)
{
    IM_ASSERT(type == ImNodesAttributeType_Input || type == ImNodesAttributeType_Output);
    const float x = type == ImNodesAttributeType_Input
                        ? (node_rect.Min.x - GImNodes->Style.PinOffset)
                        : (node_rect.Max.x + GImNodes->Style.PinOffset);
    return ImVec2(x, 0.5f * (attribute_rect.Min.y + attribute_rect.Max.y));
}

ImVec2 GetScreenSpacePinCoordinates(const ImNodesEditorContext& editor, const ImPinData& pin)
{
    const ImRect& parent_node_rect = editor.Nodes.Pool[pin.ParentNodeIdx].Rect;
    return GetScreenSpacePinCoordinates(parent_node_rect, pin.AttributeRect, pin.Type);
}

inline ImVec2 GetLabelOrigin(ImNodesEditorContext& editor, const ImLabelData& label) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImVec2 correction = window->Pos - window->Scroll; //offset due to windows data
    switch (label.parentType)
    {
    case ImNodesParentLabelType_None:
    {
        return ImVec2(0, 0);
    }
    case ImNodesParentLabelType_Pin:
    {
        const int pin_idx = ObjectPoolFind(editor.Pins, label.parentId);
        IM_ASSERT(pin_idx != -1);
        const ImPinData& pin = editor.Pins.Pool[pin_idx];

        ImVec2 origin = GetScreenSpacePinCoordinates(editor.Nodes.Pool[pin.ParentNodeIdx].Rect, pin.AttributeRect, pin.Type) - correction;

        switch (pin.Type)
        {
        case ImNodesAttributeType_None:
        {
            return origin;
        }
        case ImNodesAttributeType_Input:
        {
            return origin + ImVec2(-label.Rect.GetWidth(), -label.Rect.GetHeight());
        }
        case ImNodesAttributeType_Output:
        {
            return origin + ImVec2(label.LayoutStyle.Padding.x, -label.Rect.GetHeight());
        }
        default:
            IM_ASSERT(!"Unreachable code!");
            break;
        }

        break;
    }
    case ImNodesParentLabelType_Link:
    {
        const int link_idx = ObjectPoolFind(editor.Links, label.parentId);
        IM_ASSERT(link_idx != -1);
        const ImLinkData& link = editor.Links.Pool[link_idx];

        const ImPinData& start_pin = editor.Pins.Pool[link.StartPinIdx];
        const ImPinData& end_pin = editor.Pins.Pool[link.EndPinIdx];
        const Curve curve = GetCurve(
            start_pin.Pos, end_pin.Pos, start_pin.Type, end_pin.Type, GImNodes->Style.LinkLineSegmentsPerLength, link.LinkType, link.Deformations);

        ImVec2 origin;
        if (curve.type == ImNodesLinkType_Sloped) {
            if (curve.sloped_curve->NumSegments == 3) {
                origin = (curve.sloped_curve->P[1] + curve.sloped_curve->P[2]) * 0.5 - correction;
                return origin + ImVec2(label.LayoutStyle.Padding.x, -label.Rect.GetHeight());
            }
                
            else {
                origin = (curve.sloped_curve->P[2] + curve.sloped_curve->P[3]) * 0.5 - correction;
                return origin + ImVec2(-label.Rect.GetWidth() * 0.5, -label.Rect.GetHeight());
            }
        }
        else {
            //todo
            IM_ASSERT(!"Unreachable code!");
        }
        
    }
    default:
        IM_ASSERT(!"Unreachable code!");
        break;
    }
}

bool MouseInCanvas()
{
    // This flag should be true either when hovering or clicking something in the canvas.
    const bool is_window_hovered_or_focused = ImGui::IsWindowHovered() || ImGui::IsWindowFocused();

    return is_window_hovered_or_focused &&
           GImNodes->CanvasRectScreenSpace.Contains(ImGui::GetMousePos());
}

void BeginNodeSelection(ImNodesEditorContext& editor, const int node_idx)
{
    // Don't start selecting a node if we are e.g. already creating and dragging
    // a new link! New link creation can happen when the mouse is clicked over
    // a node, but within the hover radius of a pin.
    if (editor.ClickInteraction.Type != ImNodesClickInteractionType_None)
    {
        return;
    }

    editor.ClickInteraction.Type = ImNodesClickInteractionType_Node;
    // If the node is not already contained in the selection, then we want only
    // the interaction node to be selected, effective immediately.
    //
    // If the multiple selection modifier is active, we want to add this node
    // to the current list of selected nodes.
    //
    // Otherwise, we want to allow for the possibility of multiple nodes to be
    // moved at once.
    if (!editor.SelectedNodeIndices.contains(node_idx))
    {
        editor.SelectedLinkIndices.clear();
        editor.SelectedLabelIndices.clear();
        if (!GImNodes->MultipleSelectModifier)
        {
            editor.SelectedNodeIndices.clear();
        }
        editor.SelectedNodeIndices.push_back(node_idx);

        // Ensure that individually selected nodes get rendered on top
        ImVector<int>&   depth_stack = editor.NodeDepthOrder;
        const int* const elem = depth_stack.find(node_idx);
        IM_ASSERT(elem != depth_stack.end());
        depth_stack.erase(elem);
        depth_stack.push_back(node_idx);
    }
    // Deselect a previously-selected node
    else if (GImNodes->MultipleSelectModifier)
    {
        const int* const node_ptr = editor.SelectedNodeIndices.find(node_idx);
        editor.SelectedNodeIndices.erase(node_ptr);

        // Don't allow dragging after deselecting
        editor.ClickInteraction.Type = ImNodesClickInteractionType_None;
    }

    // To support snapping of multiple nodes, we need to store the offset of
    // each node in the selection to the origin of the dragged node.
    const ImVec2 ref_origin = editor.Nodes.Pool[node_idx].Origin;
    editor.PrimaryNodeOffset =
        ref_origin + GImNodes->CanvasOriginScreenSpace + editor.Panning - GImNodes->MousePos;

    editor.SelectedNodeOffsets.clear();
    for (int idx = 0; idx < editor.SelectedNodeIndices.Size; idx++)
    {
        const int    node = editor.SelectedNodeIndices[idx];
        const ImVec2 node_origin = editor.Nodes.Pool[node].Origin - ref_origin;
        editor.SelectedNodeOffsets.push_back(node_origin);
    }
}

void BeginLinkControlSelection(ImNodesEditorContext& editor, const int link_control_idx) {

    if (editor.ClickInteraction.Type != ImNodesClickInteractionType_None)
    {
        return;
    }

    editor.ClickInteraction.Type = ImNodesClickInteractionType_LinkControl;
    ImLinkControlData& link_control = editor.LinkControls.Pool[link_control_idx];
    //If link control not already selected, clear all selection and select only this link control
    if (!editor.SelectedLinkControlIndices.contains(link_control_idx))
    {
        editor.SelectedNodeIndices.clear();
        editor.SelectedLabelIndices.clear();
        if (!GImNodes->MultipleSelectModifier)
        {
            editor.SelectedLinkControlIndices.clear();
            editor.SelectedLinkIndices.clear();
        }
        editor.SelectedLinkControlIndices.push_back(link_control_idx);
        editor.SelectedLinkIndices.push_back(link_control.LinkIdx);
    }
    // Deselect a previously-selected node
    else if (GImNodes->MultipleSelectModifier)
    {
        const int* const link_control_ptr = editor.SelectedLinkControlIndices.find(link_control_idx);
        editor.SelectedLinkControlIndices.erase(link_control_ptr);

        // Don't allow dragging after deselecting
        editor.ClickInteraction.Type = ImNodesClickInteractionType_None;
    }

    // To support snapping of multiple nodes, we need to store the offset of
    // each node in the selection to the origin of the dragged node.
    const ImVec2 ref_origin = GetLinkControlOrigin(editor, editor.LinkControls.Pool[link_control_idx]);
    editor.PrimaryLinkControlOffset =
        ref_origin + GImNodes->CanvasOriginScreenSpace + editor.Panning - GImNodes->MousePos;
    
    editor.SelectedLinkControlOffsets.clear();
    for (int idx = 0; idx < editor.SelectedLinkControlIndices.Size; idx++)
    {
        const int    link_control = editor.SelectedLinkControlIndices[idx];
        const ImVec2 link_control_origin = GetLinkControlOrigin(editor, editor.LinkControls.Pool[link_control]) - ref_origin;
        editor.SelectedLinkControlOffsets.push_back(link_control_origin);
    }
}

void BeginLabelSelection(ImNodesEditorContext& editor, const int label_idx)
{
    if (editor.ClickInteraction.Type != ImNodesClickInteractionType_None)
    {
        return;
    }

    editor.ClickInteraction.Type = ImNodesClickInteractionType_Label;

    if (!editor.SelectedLabelIndices.contains(label_idx))
    {
        editor.SelectedLinkIndices.clear();
        editor.SelectedNodeIndices.clear();

        if (!GImNodes->MultipleSelectModifier)
        {
            editor.SelectedLabelIndices.clear();
        }
        editor.SelectedLabelIndices.push_back(label_idx);
    }
    else if (GImNodes->MultipleSelectModifier)
    {
        const int* const label_ptr = editor.SelectedLabelIndices.find(label_idx);
        editor.SelectedLabelIndices.erase(label_ptr);
        editor.ClickInteraction.Type = ImNodesClickInteractionType_None;
    }

    const ImVec2 ref_origin = GetLabelOrigin(editor, editor.Labels.Pool[label_idx]) + editor.Labels.Pool[label_idx].Deformation;
    editor.PrimaryLabelOffset =
        ref_origin  + GImNodes->CanvasOriginScreenSpace + editor.Panning - GImNodes->MousePos;

    editor.SelectedLabelOffsets.clear();
    for (int idx = 0; idx < editor.SelectedLabelIndices.Size; idx++)
    {
        const int    label = editor.SelectedLabelIndices[idx];
        const ImVec2 label_origin = GetLabelOrigin(editor, editor.Labels.Pool[label]) + editor.Labels.Pool[label].Deformation - ref_origin;
        editor.SelectedLabelOffsets.push_back(label_origin);
    }
}

ImVector<int> GetAllowedLinkControlLocalId(ImNodesEditorContext& editor, const int link_idx) {
    const ImLinkData link = editor.Links.Pool[link_idx];
    const ImPinData start_pin = editor.Pins.Pool[link.StartPinIdx];
    const ImPinData end_pin = editor.Pins.Pool[link.EndPinIdx];
    const Curve curve = GetCurve(
        start_pin.Pos, end_pin.Pos, start_pin.Type, end_pin.Type, GImNodes->Style.LinkLineSegmentsPerLength, link.LinkType, link.Deformations);

    ImVector<int> local_Ids;
    if (link.LinkType == ImNodesLinkType_Bezier) {
        local_Ids.push_back(1);
        local_Ids.push_back(4);
    }
    else if (link.LinkType == ImNodesLinkType_Sloped) {
        if (curve.sloped_curve->NumSegments == 3) {
            local_Ids.push_back(1);
            local_Ids.push_back(4);
            local_Ids.push_back(6);
            local_Ids.push_back(8);
            local_Ids.push_back(10);
        }
        else {
            local_Ids.push_back(1);
            local_Ids.push_back(2);
            local_Ids.push_back(3);
            local_Ids.push_back(4);
            local_Ids.push_back(6);
            local_Ids.push_back(7);
            local_Ids.push_back(8);
            local_Ids.push_back(9);
            local_Ids.push_back(10);
        }
    }
    return local_Ids;
}


void BeginLinkSelection(ImNodesEditorContext& editor, const int link_idx)
{
    editor.ClickInteraction.Type = ImNodesClickInteractionType_Link;
    // When a link is selected, clear all other selections, and insert the link
    // as the sole selection.
    editor.SelectedNodeIndices.clear();
    editor.SelectedLinkIndices.clear();
    editor.SelectedLinkControlIndices.clear();
    editor.SelectedLabelIndices.clear();
    editor.SelectedLinkIndices.push_back(link_idx);
}

void BeginLinkDetach(ImNodesEditorContext& editor, const int link_idx, const int detach_pin_idx)
{
    const ImLinkData&        link = editor.Links.Pool[link_idx];
    ImClickInteractionState& state = editor.ClickInteraction;
    state.Type = ImNodesClickInteractionType_LinkCreation;
    state.LinkCreation.EndPinIdx.Reset();
    state.LinkCreation.StartPinIdx =
        detach_pin_idx == link.StartPinIdx ? link.EndPinIdx : link.StartPinIdx;
    GImNodes->DeletedLinkIdx = link_idx;
}

void BeginLinkCreation(ImNodesEditorContext& editor, const int hovered_pin_idx)
{
    editor.ClickInteraction.Type = ImNodesClickInteractionType_LinkCreation;
    editor.ClickInteraction.LinkCreation.StartPinIdx = hovered_pin_idx;
    editor.ClickInteraction.LinkCreation.EndPinIdx.Reset();
    editor.ClickInteraction.LinkCreation.Type = ImNodesLinkCreationType_Standard;
    GImNodes->ImNodesUIState |= ImNodesUIState_LinkStarted;
}

void BeginLinkInteraction(
    ImNodesEditorContext& editor,
    const int             link_idx,
    const ImOptionalIndex pin_idx = ImOptionalIndex())
{
    // Check if we are clicking the link with the modifier pressed.
    // This will in a link detach via clicking.

    const bool modifier_pressed = GImNodes->Io.LinkDetachWithModifierClick.Modifier == NULL
                                      ? false
                                      : *GImNodes->Io.LinkDetachWithModifierClick.Modifier;

    if (modifier_pressed)
    {
        const ImLinkData& link = editor.Links.Pool[link_idx];
        const ImPinData&  start_pin = editor.Pins.Pool[link.StartPinIdx];
        const ImPinData&  end_pin = editor.Pins.Pool[link.EndPinIdx];
        const ImVec2&     mouse_pos = GImNodes->MousePos;
        const float       dist_to_start = ImLengthSqr(start_pin.Pos - mouse_pos);
        const float       dist_to_end = ImLengthSqr(end_pin.Pos - mouse_pos);
        const int closest_pin_idx = dist_to_start < dist_to_end ? link.StartPinIdx : link.EndPinIdx;

        editor.ClickInteraction.Type = ImNodesClickInteractionType_LinkCreation;
        BeginLinkDetach(editor, link_idx, closest_pin_idx);
        editor.ClickInteraction.LinkCreation.Type = ImNodesLinkCreationType_FromDetach;
    }
    else
    {
        if (pin_idx.HasValue())
        {
            const int hovered_pin_flags = editor.Pins.Pool[pin_idx.Value()].Flags;

            // Check the 'click and drag to detach' case.
            if (hovered_pin_flags & ImNodesAttributeFlags_EnableLinkDetachWithDragClick)
            {
                BeginLinkDetach(editor, link_idx, pin_idx.Value());
                editor.ClickInteraction.LinkCreation.Type = ImNodesLinkCreationType_FromDetach;
            }
            else
            {
                BeginLinkCreation(editor, pin_idx.Value());
            }
        }
        else
        {
            BeginLinkSelection(editor, link_idx);
        }
    }
}

static inline bool IsMiniMapHovered();

void BeginCanvasInteraction(ImNodesEditorContext& editor)
{
    const bool any_ui_element_hovered =
        GImNodes->HoveredNodeIdx.HasValue() || GImNodes->HoveredLinkIdx.HasValue() ||
        GImNodes->HoveredPinIdx.HasValue() || GImNodes->HoveredLinkControlIdx.HasValue() || ImGui::IsAnyItemHovered();

    const bool mouse_not_in_canvas = !MouseInCanvas();

    if (editor.ClickInteraction.Type != ImNodesClickInteractionType_None ||
        any_ui_element_hovered || mouse_not_in_canvas)
    {
        return;
    }

    const bool started_panning = GImNodes->AltMouseClicked;

    if (started_panning)
    {
        editor.ClickInteraction.Type = ImNodesClickInteractionType_Panning;
    }
    else if (GImNodes->LeftMouseClicked)
    {
        editor.ClickInteraction.Type = ImNodesClickInteractionType_BoxSelection;
        editor.ClickInteraction.BoxSelector.Rect.Min =
            ScreenSpaceToGridSpace(editor, GImNodes->MousePos);
    }
}

void BoxSelectorUpdateSelection(ImNodesEditorContext& editor, ImRect box_rect)
{
    // Invert box selector coordinates as needed

    if (box_rect.Min.x > box_rect.Max.x)
    {
        ImSwap(box_rect.Min.x, box_rect.Max.x);
    }

    if (box_rect.Min.y > box_rect.Max.y)
    {
        ImSwap(box_rect.Min.y, box_rect.Max.y);
    }

    // Update node selection

    editor.SelectedNodeIndices.clear();

    // Test for overlap against node rectangles

    for (int node_idx = 0; node_idx < editor.Nodes.Pool.size(); ++node_idx)
    {
        if (editor.Nodes.InUse[node_idx])
        {
            ImNodeData& node = editor.Nodes.Pool[node_idx];
            if (box_rect.Overlaps(node.Rect))
            {
                editor.SelectedNodeIndices.push_back(node_idx);
            }
        }
    }

    // Update link selection

    editor.SelectedLinkIndices.clear();
    editor.SelectedLinkControlIndices.clear();
    editor.SelectedLabelIndices.clear();

    // Test for overlap against links

    for (int link_idx = 0; link_idx < editor.Links.Pool.size(); ++link_idx)
    {
        if (editor.Links.InUse[link_idx])
        {
            const ImLinkData& link = editor.Links.Pool[link_idx];

            const ImPinData& pin_start = editor.Pins.Pool[link.StartPinIdx];
            const ImPinData& pin_end = editor.Pins.Pool[link.EndPinIdx];
            const ImRect&    node_start_rect = editor.Nodes.Pool[pin_start.ParentNodeIdx].Rect;
            const ImRect&    node_end_rect = editor.Nodes.Pool[pin_end.ParentNodeIdx].Rect;

            const ImVec2 start = GetScreenSpacePinCoordinates(
                node_start_rect, pin_start.AttributeRect, pin_start.Type);
            const ImVec2 end =
                GetScreenSpacePinCoordinates(node_end_rect, pin_end.AttributeRect, pin_end.Type);

            // Test
            if (RectangleOverlapsLink(box_rect, start, end, pin_start.Type, link.LinkType, link.Deformations))
            {
                editor.SelectedLinkIndices.push_back(link_idx);
            }
        }
    }

    if (editor.SelectedNodeIndices.Size == 0) {
        for (int link_control_idx = 0; link_control_idx < editor.LinkControls.Pool.Size; link_control_idx++) {

            if (!editor.LinkControls.InUse[link_control_idx])
                continue;

            ImLinkControlData& link_control = editor.LinkControls.Pool[link_control_idx];

            const ImLinkData& link = editor.Links.Pool[link_control.LinkIdx];
            const ImPinData& start_pin = editor.Pins.Pool[link.StartPinIdx];
            const ImPinData& end_pin = editor.Pins.Pool[link.EndPinIdx];
            const Curve curve = GetCurve(
                start_pin.Pos, end_pin.Pos, start_pin.Type, end_pin.Type, GImNodes->Style.LinkLineSegmentsPerLength, link.LinkType, link.Deformations);

            ControlPrimitive cp = GetControlPrimitive(link_control.LocalId, curve);
            if (RectangleOverlapsControlPrimitive(box_rect, cp)) {
                editor.SelectedLinkControlIndices.push_back(link_control_idx);
            }
        }

        for (int label_idx = 0; label_idx < editor.Labels.Pool.Size; label_idx++) {
            if (!editor.Labels.InUse[label_idx])
                continue;

            if (box_rect.Overlaps(editor.Labels.Pool[label_idx].Rect))
                editor.SelectedLabelIndices.push_back(label_idx);
        }
    }
}

ImVec2 SnapOriginToGrid(ImVec2 origin)
{
    if (GImNodes->Style.Flags & ImNodesStyleFlags_GridSnapping)
    {
        const float spacing = GImNodes->Style.GridSpacing;
        const float spacing2 = spacing * 0.5f;

        // Snap the origin to the nearest grid point in any direction
        float modx = fmodf(fabsf(origin.x) + spacing2, spacing) - spacing2;
        float mody = fmodf(fabsf(origin.y) + spacing2, spacing) - spacing2;
        origin.x += (origin.x < 0.f) ? modx : -modx;
        origin.y += (origin.y < 0.f) ? mody : -mody;
    }

    return origin;
}

void TranslateSelectedNodes(ImNodesEditorContext& editor)
{
    if (editor.current_event.Event == -1) {
        editor.current_event = ImNodesEventVarElement(ImNodesEventVar_NodeMove);
        for (int i = 0; i < editor.SelectedNodeIndices.Size; i++) {
            ImNodeData& node = editor.Nodes.Pool[editor.SelectedNodeIndices[i]];
            editor.current_event.addOldPos(node.Id, node.Origin);
        }
    }
    if (GImNodes->LeftMouseDragging)
    {
        // If we have grid snap enabled, don't start moving nodes until we've moved the mouse
        // slightly
        const bool shouldTranslate = (GImNodes->Style.Flags & ImNodesStyleFlags_GridSnapping)
                                         ? ImGui::GetIO().MouseDragMaxDistanceSqr[0] > 5.0
                                         : true;

        const ImVec2 origin = SnapOriginToGrid(
            GImNodes->MousePos - GImNodes->CanvasOriginScreenSpace - editor.Panning +
            editor.PrimaryNodeOffset);
        for (int i = 0; i < editor.SelectedNodeIndices.size(); ++i)
        {
            const ImVec2 node_rel = editor.SelectedNodeOffsets[i];
            const int    node_idx = editor.SelectedNodeIndices[i];
            ImNodeData&  node = editor.Nodes.Pool[node_idx];
            if (node.Draggable && shouldTranslate)
            {
                node.Origin = origin + node_rel + editor.AutoPanningDelta;
            }
        }
    }
}

void TranslateSelectedLinkControl(ImNodesEditorContext& editor)
{
    if (editor.current_event.Event == -1) {
        editor.current_event = ImNodesEventVarElement(ImNodesEventVar_LinkDeformation);
        for (int i = 0; i < editor.SelectedLinkControlIndices.Size; i++) {
            const ImLinkControlData& link_control = editor.LinkControls.Pool[editor.SelectedLinkControlIndices[i]];

            const ImLinkData& link = editor.Links.Pool[link_control.LinkIdx];
            IM_ASSERT(link_control.Id == GetLinkControlId(link_control.LocalId, link.Id));

            int local_Ids[2];
            const ImPinData& start_pin = editor.Pins.Pool[link.StartPinIdx];
            const ImPinData& end_pin = editor.Pins.Pool[link.EndPinIdx];
            const Curve curve = GetCurve(
                start_pin.Pos, end_pin.Pos, start_pin.Type, end_pin.Type, GImNodes->Style.LinkLineSegmentsPerLength, link.LinkType, link.Deformations);
            int nb_point = GetControlPrimitivePointsLocalID(link_control.LocalId, *curve.sloped_curve, local_Ids);
            if (nb_point < 2)
                IM_ASSERT(local_Ids[0] < 6 && local_Ids[0] >= 0);
            if (nb_point == 2)
                IM_ASSERT(local_Ids[1] < 6 && local_Ids[1] >= 0);

            for (int j = 0; j < nb_point; j++) {
                if (!editor.current_event.Ids.contains(GetLinkControlId(local_Ids[j], link.Id))) {
                    //need to check if id already in if we add segment and point Id, could get duplicates
                    editor.current_event.addOldPos(GetLinkControlId(local_Ids[j], link.Id), link.Deformations[local_Ids[j]]);
                }
            }
        }
    }

    if (GImNodes->LeftMouseDragging)
    {
        // If we have grid snap enabled, don't start moving nodes until we've moved the mouse
        // slightly
        const bool shouldTranslate = (GImNodes->Style.Flags & ImNodesStyleFlags_GridSnapping)
            ? ImGui::GetIO().MouseDragMaxDistanceSqr[0] > 5.0
            : true;

        const ImVec2 origin = SnapOriginToGrid(
            GImNodes->MousePos - GImNodes->CanvasOriginScreenSpace - editor.Panning +
            editor.PrimaryLinkControlOffset);
        for (int i = 0; i < editor.SelectedLinkControlIndices.size(); ++i)
        {
            const ImVec2 link_control_rel = editor.SelectedLinkControlOffsets[i];
            const int    link_control_idx = editor.SelectedLinkControlIndices[i];
            ImLinkControlData& link_control = editor.LinkControls.Pool[link_control_idx];
            if (shouldTranslate)
            {
                MooveLinkControl(editor, link_control, origin + link_control_rel + editor.AutoPanningDelta);
            }
        }
    }
}

void TranslateSelectedLabels(ImNodesEditorContext& editor)
{
    if (editor.current_event.Event == -1) {
        editor.current_event = ImNodesEventVarElement(ImNodesEventVar_LabelMove);
        for (int i = 0; i < editor.SelectedLabelIndices.Size; i++) {
            ImLabelData& label = editor.Labels.Pool[editor.SelectedLabelIndices[i]];
            editor.current_event.addOldPos(label.Id, label.Deformation);
        }
    }
    if (GImNodes->LeftMouseDragging)
    {
        // If we have grid snap enabled, don't start moving nodes until we've moved the mouse
        // slightly
        const bool shouldTranslate = (GImNodes->Style.Flags & ImNodesStyleFlags_GridSnapping)
            ? ImGui::GetIO().MouseDragMaxDistanceSqr[0] > 5.0
            : true;

        const ImVec2 origin = SnapOriginToGrid(
            GImNodes->MousePos - GImNodes->CanvasOriginScreenSpace - editor.Panning +
            editor.PrimaryLabelOffset);
        for (int i = 0; i < editor.SelectedLabelIndices.size(); ++i)
        {
            const ImVec2 label_rel = editor.SelectedLabelOffsets[i];
            const int    label_idx = editor.SelectedLabelIndices[i];
            ImLabelData& label = editor.Labels.Pool[label_idx];
            if (label.Draggable && shouldTranslate)
            {
                label.Deformation = origin + label_rel + editor.AutoPanningDelta - GetLabelOrigin(editor, label);
            }
        }
    }
}

struct LinkPredicate
{
    bool operator()(const ImLinkData& lhs, const ImLinkData& rhs) const
    {
        // Do a unique compare by sorting the pins' addresses.
        // This catches duplicate links, whether they are in the
        // same direction or not.
        // Sorting by pin index should have the uniqueness guarantees as sorting
        // by id -- each unique id will get one slot in the link pool array.

        int lhs_start = lhs.StartPinIdx;
        int lhs_end = lhs.EndPinIdx;
        int rhs_start = rhs.StartPinIdx;
        int rhs_end = rhs.EndPinIdx;

        if (lhs_start > lhs_end)
        {
            ImSwap(lhs_start, lhs_end);
        }

        if (rhs_start > rhs_end)
        {
            ImSwap(rhs_start, rhs_end);
        }

        return lhs_start == rhs_start && lhs_end == rhs_end;
    }
};

ImOptionalIndex FindDuplicateLink(
    const ImNodesEditorContext& editor,
    const int                   start_pin_idx,
    const int                   end_pin_idx)
{
    ImLinkData test_link(0);
    test_link.StartPinIdx = start_pin_idx;
    test_link.EndPinIdx = end_pin_idx;
    for (int link_idx = 0; link_idx < editor.Links.Pool.size(); ++link_idx)
    {
        const ImLinkData& link = editor.Links.Pool[link_idx];
        if (LinkPredicate()(test_link, link) && editor.Links.InUse[link_idx])
        {
            return ImOptionalIndex(link_idx);
        }
    }

    return ImOptionalIndex();
}

bool ShouldLinkSnapToPin(
    const ImNodesEditorContext& editor,
    const ImPinData& start_pin,
    const int                   hovered_pin_idx,
    const ImOptionalIndex       duplicate_link)
{
    const ImPinData& end_pin = editor.Pins.Pool[hovered_pin_idx];

    // The end pin must be in a different node
    /*if (start_pin.ParentNodeIdx == end_pin.ParentNodeIdx)
    {
        return false;
    }*/

    // The end pin must be of a different type
    if (start_pin.Type == end_pin.Type)
    {
        return false;
    }

    // The link to be created must not be a duplicate, unless it is the link which was created on
    // snap. In that case we want to snap, since we want it to appear visually as if the created
    // link remains snapped to the pin.
    if (duplicate_link.HasValue() && !(duplicate_link == GImNodes->SnapLinkIdx))
    {
        return false;
    }

    return true;
}

struct ImNodesEventVarInfo
{
    ImGuiDataType Type; // not usefull as we can have a mixe of type...
    ImU32         IntCount;
    ImU32         FloatCount;

};

static void PushEventVar(const ImNodesEventVarElement& element)
{
    GImNodes->EventStack.push(element);
}

void PopEventVar()
{
    ImNodesEventVarElement dest;
    if (!GImNodes->EventStack.pop(&dest))
        return;
    ImNodesEditorContext& editor = EditorContextGet();
    switch (dest.Event)
    {
    case ImNodesEventVar_LinkDeformation:
    {
        IM_ASSERT(dest.Ids.size() == dest.OldPoss.size());
        IM_ASSERT(dest.Ids.size() == dest.NewPoss.size());
        for (int i = 0; i < dest.Ids.size(); i++) {
            int Id = dest.Ids[i];
            int linkId = GetLinkControlLinkId(Id);
            int localId = GetLinkControlLocalId(Id);

            const int link_idx = ObjectPoolFind(editor.Links, linkId);
            IM_ASSERT(link_idx != -1);
            ImLinkData& link = editor.Links.Pool[link_idx];

            link.Deformations[localId] = dest.OldPoss[i];
        }
        break;
    }
    case ImNodesEventVar_NodeMove:
    {
        IM_ASSERT(dest.Ids.size() == dest.OldPoss.size());
        IM_ASSERT(dest.Ids.size() == dest.NewPoss.size());
        for (int i = 0; i < dest.Ids.size(); i++) {
            int Id = dest.Ids[i];
            const int node_idx = ObjectPoolFind(editor.Nodes, Id);
            IM_ASSERT(node_idx != -1);
            ImNodeData& data = editor.Nodes.Pool[node_idx];
            data.Origin = dest.OldPoss[i];
        }
        break;
    }
    case ImNodesEventVar_LabelMove:
    {
        IM_ASSERT(dest.Ids.size() == dest.OldPoss.size());
        IM_ASSERT(dest.Ids.size() == dest.NewPoss.size());
        for (int i = 0; i < dest.Ids.size(); i++) {
            int Id = dest.Ids[i];
            const int label_idx = ObjectPoolFind(editor.Labels, Id);
            IM_ASSERT(label_idx != -1);
            ImLabelData& data = editor.Labels.Pool[label_idx];
            data.Deformation = dest.OldPoss[i];
        }
        break;
    }
    case ImNodesEventVar_UserEvent:
        GImNodes->PopedEvent = dest.GetId();
        break;
    default:
        break;
    }
}

void UnpopEventVar()
{
    ImNodesEventVarElement dest;
    if (!GImNodes->EventStack.unpop(&dest))
        return;

    ImNodesEditorContext& editor = EditorContextGet();
    switch (dest.Event)
    {
    case ImNodesEventVar_LinkDeformation:
    {
        IM_ASSERT(dest.Ids.size() == dest.OldPoss.size());
        IM_ASSERT(dest.Ids.size() == dest.NewPoss.size());
        for (int i = 0; i < dest.Ids.size(); i++) {
            int Id = dest.Ids[i];
            int linkId = GetLinkControlLinkId(Id);
            int localId = GetLinkControlLocalId(Id);

            const int link_idx = ObjectPoolFind(editor.Links, linkId);
            IM_ASSERT(link_idx != -1);
            ImLinkData& link = editor.Links.Pool[link_idx];


            link.Deformations[localId] = dest.NewPoss[i];
        }
        break;
    }
    case ImNodesEventVar_NodeMove:
    {
        IM_ASSERT(dest.Ids.size() == dest.OldPoss.size());
        IM_ASSERT(dest.Ids.size() == dest.NewPoss.size());
        for (int i = 0; i < dest.Ids.size(); i++) {
            int Id = dest.Ids[i];
            const int node_idx = ObjectPoolFind(editor.Nodes, Id);
            IM_ASSERT(node_idx != -1);
            ImNodeData& data = editor.Nodes.Pool[node_idx];
            data.Origin = dest.NewPoss[i];
        }
        break;
    }
    case ImNodesEventVar_LabelMove:
    {
        IM_ASSERT(dest.Ids.size() == dest.OldPoss.size());
        IM_ASSERT(dest.Ids.size() == dest.NewPoss.size());
        for (int i = 0; i < dest.Ids.size(); i++) {
            int Id = dest.Ids[i];
            const int label_idx = ObjectPoolFind(editor.Labels, Id);
            IM_ASSERT(label_idx != -1);
            ImLabelData& data = editor.Labels.Pool[label_idx];
            data.Deformation = dest.NewPoss[i];
        }
        break;
    }
    case ImNodesEventVar_UserEvent:
        GImNodes->UnpopedEvent = dest.GetId();
        break;
    default:
        break;
    }
}

void ClickInteractionUpdate(ImNodesEditorContext& editor)
{
    switch (editor.ClickInteraction.Type)
    {
    case ImNodesClickInteractionType_BoxSelection:
    {
        editor.ClickInteraction.BoxSelector.Rect.Max =
            ScreenSpaceToGridSpace(editor, GImNodes->MousePos);

        ImRect box_rect = editor.ClickInteraction.BoxSelector.Rect;
        box_rect.Min = GridSpaceToScreenSpace(editor, box_rect.Min);
        box_rect.Max = GridSpaceToScreenSpace(editor, box_rect.Max);

        BoxSelectorUpdateSelection(editor, box_rect);

        const ImU32 box_selector_color = GImNodes->Style.Colors[ImNodesCol_BoxSelector];
        const ImU32 box_selector_outline = GImNodes->Style.Colors[ImNodesCol_BoxSelectorOutline];
        GImNodes->CanvasDrawList->AddRectFilled(box_rect.Min, box_rect.Max, box_selector_color);
        GImNodes->CanvasDrawList->AddRect(box_rect.Min, box_rect.Max, box_selector_outline);

        if (GImNodes->LeftMouseReleased)
        {
            ImVector<int>& depth_stack = editor.NodeDepthOrder;
            const ImVector<int>& selected_idxs = editor.SelectedNodeIndices;

            // Bump the selected node indices, in order, to the top of the depth stack.
            // NOTE: this algorithm has worst case time complexity of O(N^2), if the node selection
            // is ~ N (due to selected_idxs.contains()).

            if ((selected_idxs.Size > 0) && (selected_idxs.Size < depth_stack.Size))
            {
                int num_moved = 0; // The number of indices moved. Stop after selected_idxs.Size
                for (int i = 0; i < depth_stack.Size - selected_idxs.Size; ++i)
                {
                    for (int node_idx = depth_stack[i]; selected_idxs.contains(node_idx);
                        node_idx = depth_stack[i])
                    {
                        depth_stack.erase(depth_stack.begin() + static_cast<size_t>(i));
                        depth_stack.push_back(node_idx);
                        ++num_moved;
                    }

                    if (num_moved == selected_idxs.Size)
                    {
                        break;
                    }
                }
            }

            editor.ClickInteraction.Type = ImNodesClickInteractionType_None;
        }
    }
    break;
    case ImNodesClickInteractionType_Node:
    {
        TranslateSelectedNodes(editor);

        if (GImNodes->LeftMouseReleased)
        {
            editor.ClickInteraction.Type = ImNodesClickInteractionType_None;
            for (int i = 0; i < editor.SelectedNodeIndices.size(); i++) {
                const ImNodeData& data = editor.Nodes.Pool[editor.SelectedNodeIndices[i]];

                editor.current_event.addNewPos(data.Id, data.Origin);
            }
            if (editor.current_event.valid())
                PushEventVar(editor.current_event);
            editor.current_event = ImNodesEventVarElement();
        }
    }
    break;
    case ImNodesClickInteractionType_LinkControl:
    {


        TranslateSelectedLinkControl(editor);

        if (ImGui::IsMouseDoubleClicked(0)) {
            ContraintLinkControl(editor, editor.LinkControls.Pool[GImNodes->HoveredLinkControlIdx.Value()]);
        }
        else if (GImNodes->LeftMouseReleased)
        {
            editor.ClickInteraction.Type = ImNodesClickInteractionType_None;

            {
                IM_ASSERT(editor.current_event.Event == ImNodesEventVar_LinkDeformation);

                for (int i = 0; i < editor.SelectedLinkControlIndices.Size; i++) {
                    const ImLinkControlData& link_control = editor.LinkControls.Pool[editor.SelectedLinkControlIndices[i]];

                    const ImLinkData& link = editor.Links.Pool[link_control.LinkIdx];
                    IM_ASSERT(link_control.Id == GetLinkControlId(link_control.LocalId, link.Id));

                    int local_Ids[2];
                    const ImPinData& start_pin = editor.Pins.Pool[link.StartPinIdx];
                    const ImPinData& end_pin = editor.Pins.Pool[link.EndPinIdx];
                    const Curve curve = GetCurve(
                        start_pin.Pos, end_pin.Pos, start_pin.Type, end_pin.Type, GImNodes->Style.LinkLineSegmentsPerLength, link.LinkType, link.Deformations);
                    int nb_point = GetControlPrimitivePointsLocalID(link_control.LocalId, *curve.sloped_curve, local_Ids);

                    for (int j = 0; j < nb_point; j++) {
                        editor.current_event.addNewPos(GetLinkControlId(local_Ids[j], link.Id), link.Deformations[local_Ids[j]]);
                    }
                }

                if (editor.current_event.valid())
                    PushEventVar(editor.current_event);
                editor.current_event = ImNodesEventVarElement();
            }         
        }
    }
    break;
    case ImNodesClickInteractionType_Label:
    {
        TranslateSelectedLabels(editor);

        if (ImGui::IsMouseDoubleClicked(0)) {
            ContraintLabel(editor, editor.Labels.Pool[GImNodes->HoveredLabelIdx.Value()]);
        }
        if (GImNodes->LeftMouseReleased)
        {
            editor.ClickInteraction.Type = ImNodesClickInteractionType_None;
            for (int i = 0; i < editor.SelectedLabelIndices.size(); i++) {
                const ImLabelData& data = editor.Labels.Pool[editor.SelectedLabelIndices[i]];

                editor.current_event.addNewPos(data.Id, data.Deformation);
            }
            if (editor.current_event.valid())
                PushEventVar(editor.current_event);
            editor.current_event = ImNodesEventVarElement();
        }
        break;
    }
    case ImNodesClickInteractionType_Link:
    {
        if (GImNodes->LeftMouseReleased)
        {
            editor.ClickInteraction.Type = ImNodesClickInteractionType_None;
        }
    }
    break;
    case ImNodesClickInteractionType_LinkCreation:
    {
        const ImPinData& start_pin =
            editor.Pins.Pool[editor.ClickInteraction.LinkCreation.StartPinIdx];
        const ImOptionalIndex maybe_duplicate_link_idx =
            GImNodes->HoveredPinIdx.HasValue()
                ? FindDuplicateLink(
                      editor,
                      editor.ClickInteraction.LinkCreation.StartPinIdx,
                      GImNodes->HoveredPinIdx.Value())
                : ImOptionalIndex();

        const bool should_snap =
            GImNodes->HoveredPinIdx.HasValue() &&
            ShouldLinkSnapToPin(
                editor, start_pin, GImNodes->HoveredPinIdx.Value(), maybe_duplicate_link_idx);

        // If we created on snap and the hovered pin is empty or changed, then we need signal that
        // the link's state has changed.
        const bool snapping_pin_changed =
            editor.ClickInteraction.LinkCreation.EndPinIdx.HasValue() &&
            !(GImNodes->HoveredPinIdx == editor.ClickInteraction.LinkCreation.EndPinIdx);

        // Detach the link that was created by this link event if it's no longer in snap range
        if (snapping_pin_changed && GImNodes->SnapLinkIdx.HasValue())
        {
            BeginLinkDetach(
                editor,
                GImNodes->SnapLinkIdx.Value(),
                editor.ClickInteraction.LinkCreation.EndPinIdx.Value());
        }

        const ImVec2 start_pos = GetScreenSpacePinCoordinates(editor, start_pin);
        // If we are within the hover radius of a receiving pin, snap the link
        // endpoint to it
        const ImVec2 end_pos = should_snap
                                   ? GetScreenSpacePinCoordinates(
                                         editor, editor.Pins.Pool[GImNodes->HoveredPinIdx.Value()])
                                   : GImNodes->MousePos;
        //editor.Pins.Pool[editor.ClickInteraction.LinkCreation.EndPinIdx.Value()]
        ImNodesLinkDeformations no_deformation;
        Curve curve = GetCurve(
            start_pos, end_pos, start_pin.Type, GImNodes->Style.LinkLineSegmentsPerLength, GImNodes->Style.LinkCreationType, no_deformation);
        if (GImNodes->HoveredPinIdx.HasValue()) {
            curve = GetCurve(
                start_pos, end_pos, start_pin.Type, editor.Pins.Pool[GImNodes->HoveredPinIdx.Value()].Type, GImNodes->Style.LinkLineSegmentsPerLength, GImNodes->Style.LinkCreationType, no_deformation);
        }

        DrawCurve(curve, GImNodes->Style.Colors[ImNodesCol_Link], GImNodes->Style.LinkThickness);
       
        const bool link_creation_on_snap =
            GImNodes->HoveredPinIdx.HasValue() &&
            (editor.Pins.Pool[GImNodes->HoveredPinIdx.Value()].Flags &
             ImNodesAttributeFlags_EnableLinkCreationOnSnap);

        if (!should_snap)
        {
            editor.ClickInteraction.LinkCreation.EndPinIdx.Reset();
        }

        const bool create_link =
            should_snap && (GImNodes->LeftMouseReleased || link_creation_on_snap);

        if (create_link && !maybe_duplicate_link_idx.HasValue())
        {
            // Avoid send OnLinkCreated() events every frame if the snap link is not saved
            // (only applies for EnableLinkCreationOnSnap)
            if (!GImNodes->LeftMouseReleased &&
                editor.ClickInteraction.LinkCreation.EndPinIdx == GImNodes->HoveredPinIdx)
            {
                break;
            }

            GImNodes->ImNodesUIState |= ImNodesUIState_LinkCreated;
            editor.ClickInteraction.LinkCreation.EndPinIdx = GImNodes->HoveredPinIdx.Value();
        }

        if (GImNodes->LeftMouseReleased)
        {
            editor.ClickInteraction.Type = ImNodesClickInteractionType_None;
            if (!create_link)
            {
                GImNodes->ImNodesUIState |= ImNodesUIState_LinkDropped;
            }
        }
    }
    break;
    case ImNodesClickInteractionType_Panning:
    {
        const bool dragging = GImNodes->AltMouseDragging;

        if (dragging)
        {
            editor.Panning += ImGui::GetIO().MouseDelta;
        }
        else
        {
            editor.ClickInteraction.Type = ImNodesClickInteractionType_None;
        }
    }
    break;
    case ImNodesClickInteractionType_ImGuiItem:
    {
        if (GImNodes->LeftMouseReleased)
        {
            editor.ClickInteraction.Type = ImNodesClickInteractionType_None;
        }
    }
    case ImNodesClickInteractionType_None:
        break;
    default:
        IM_ASSERT(!"Unreachable code!");
        break;
    }
}

void ResolveOccludedPins(const ImNodesEditorContext& editor, ImVector<int>& occluded_pin_indices)
{
    const ImVector<int>& depth_stack = editor.NodeDepthOrder;

    occluded_pin_indices.resize(0);

    if (depth_stack.Size < 2)
    {
        return;
    }

    // For each node in the depth stack
    for (int depth_idx = 0; depth_idx < (depth_stack.Size - 1); ++depth_idx)
    {
        const ImNodeData& node_below = editor.Nodes.Pool[depth_stack[depth_idx]];

        // Iterate over the rest of the depth stack to find nodes overlapping the pins
        for (int next_depth_idx = depth_idx + 1; next_depth_idx < depth_stack.Size;
             ++next_depth_idx)
        {
            const ImRect& rect_above = editor.Nodes.Pool[depth_stack[next_depth_idx]].Rect;

            // Iterate over each pin
            for (int idx = 0; idx < node_below.PinIndices.Size; ++idx)
            {
                const int     pin_idx = node_below.PinIndices[idx];
                const ImVec2& pin_pos = editor.Pins.Pool[pin_idx].Pos;

                if (rect_above.Contains(pin_pos))
                {
                    occluded_pin_indices.push_back(pin_idx);
                }
            }
        }
    }
}

ImOptionalIndex ResolveHoveredPin(
    const ImObjectPool<ImPinData>& pins,
    const ImVector<int>&           occluded_pin_indices)
{
    float           smallest_distance = FLT_MAX;
    ImOptionalIndex pin_idx_with_smallest_distance;

    const float hover_radius_sqr = GImNodes->Style.PinHoverRadius * GImNodes->Style.PinHoverRadius;

    for (int idx = 0; idx < pins.Pool.Size; ++idx)
    {
        if (!pins.InUse[idx])
        {
            continue;
        }

        if (occluded_pin_indices.contains(idx))
        {
            continue;
        }

        const ImVec2& pin_pos = pins.Pool[idx].Pos;
        const float   distance_sqr = ImLengthSqr(pin_pos - GImNodes->MousePos);

        // TODO: GImNodes->Style.PinHoverRadius needs to be copied into pin data and the pin-local
        // value used here. This is no longer called in BeginAttribute/EndAttribute scope and the
        // detected pin might have a different hover radius than what the user had when calling
        // BeginAttribute/EndAttribute.
        if (distance_sqr < hover_radius_sqr && distance_sqr < smallest_distance)
        {
            smallest_distance = distance_sqr;
            pin_idx_with_smallest_distance = idx;
        }
    }

    return pin_idx_with_smallest_distance;
}

ImOptionalIndex ResolveHoveredNode(const ImVector<int>& depth_stack)
{
    if (GImNodes->NodeIndicesOverlappingWithMouse.size() == 0)
    {
        return ImOptionalIndex();
    }

    if (GImNodes->NodeIndicesOverlappingWithMouse.size() == 1)
    {
        return ImOptionalIndex(GImNodes->NodeIndicesOverlappingWithMouse[0]);
    }

    int largest_depth_idx = -1;
    int node_idx_on_top = -1;

    for (int i = 0; i < GImNodes->NodeIndicesOverlappingWithMouse.size(); ++i)
    {
        const int node_idx = GImNodes->NodeIndicesOverlappingWithMouse[i];
        for (int depth_idx = 0; depth_idx < depth_stack.size(); ++depth_idx)
        {
            if (depth_stack[depth_idx] == node_idx && (depth_idx > largest_depth_idx))
            {
                largest_depth_idx = depth_idx;
                node_idx_on_top = node_idx;
            }
        }
    }

    IM_ASSERT(node_idx_on_top != -1);
    return ImOptionalIndex(node_idx_on_top);
}

ImOptionalIndex ResolveHoveredLink(
    const ImObjectPool<ImLinkData>& links,
    const ImObjectPool<ImPinData>&  pins)
{
    float           smallest_distance = FLT_MAX;
    ImOptionalIndex link_idx_with_smallest_distance;

    // There are two ways a link can be detected as "hovered".
    // 1. The link is within hover distance to the mouse. The closest such link is selected as being
    // hovered over.
    // 2. If the link is connected to the currently hovered pin.
    //
    // The latter is a requirement for link detaching with drag click to work, as both a link and
    // pin are required to be hovered over for the feature to work.

    for (int idx = 0; idx < links.Pool.Size; ++idx)
    {
        if (!links.InUse[idx])
        {
            continue;
        }

        const ImLinkData& link = links.Pool[idx];
        const ImPinData&  start_pin = pins.Pool[link.StartPinIdx];
        const ImPinData&  end_pin = pins.Pool[link.EndPinIdx];
        
        // If there is a hovered pin links can only be considered hovered if they use that pin
        if (GImNodes->HoveredPinIdx.HasValue())
        {
            if (GImNodes->HoveredPinIdx == link.StartPinIdx ||
                GImNodes->HoveredPinIdx == link.EndPinIdx)
            {
                return idx;
            }
            continue;
        }

        // TODO: the calculated CubicBeziers could be cached since we generate them again when
        // rendering the links

        const Curve curve = GetCurve(
            start_pin.Pos, end_pin.Pos, start_pin.Type, end_pin.Type, GImNodes->Style.LinkLineSegmentsPerLength, link.LinkType, link.Deformations);

        // The distance test
        {
            const ImRect link_rect = GetContainingRectForCurve(curve);

            // First, do a simple bounding box test against the box containing the link
            // to see whether calculating the distance to the link is worth doing.
            if (link_rect.Contains(GImNodes->MousePos))
            {
                const float distance = GetDistanceToCurve(
                    GImNodes->MousePos, curve, CurveNumSegments(curve));

                // TODO: GImNodes->Style.LinkHoverDistance could be also copied into ImLinkData,
                // since we're not calling this function in the same scope as ImNodes::Link(). The
                // rendered/detected link might have a different hover distance than what the user
                // had specified when calling Link()
                if (distance < GImNodes->Style.LinkHoverDistance && distance < smallest_distance)
                {
                    smallest_distance = distance;
                    link_idx_with_smallest_distance = idx;
                }
            }
        }
    }

    return link_idx_with_smallest_distance;
}

ImOptionalIndex ResolveHoveredLinkControl(
    const ImObjectPool<ImLinkControlData>& linkControls,
    const ImObjectPool<ImLinkData>& links,
    const ImObjectPool<ImPinData>& pins)
{
    float           smallest_distance = FLT_MAX;
    ImOptionalIndex control_primitive_idx_with_smallest_distance;
    ImNodesLinkControlType control_primitive_type = ImNodesLinkControlType_Segment;

    for (int idx = 0; idx < linkControls.Pool.Size; ++idx)
    {

        if (!linkControls.InUse[idx])
        {
            continue;
        }

        const ImLinkControlData control_primitive = linkControls.Pool[idx];
        const ImLinkData& link = links.Pool[control_primitive.LinkIdx];
        const ImPinData& start_pin = pins.Pool[link.StartPinIdx];
        const ImPinData& end_pin = pins.Pool[link.EndPinIdx];

        const Curve curve = GetCurve(
            start_pin.Pos, end_pin.Pos, start_pin.Type, end_pin.Type, GImNodes->Style.LinkLineSegmentsPerLength, link.LinkType, link.Deformations);

        const ControlPrimitive cp = GetControlPrimitive(control_primitive.LocalId, curve);

        // The distance test
        {
            const float distance = GetDistanceToControlPrimitive(
                GImNodes->MousePos, cp);


            if (distance < GImNodes->Style.LinkHoverDistance) {
                if (control_primitive_type == cp.Type && distance < smallest_distance) {
                    smallest_distance = distance;
                    control_primitive_idx_with_smallest_distance = idx;
                }
                else if (control_primitive_type == ImNodesLinkControlType_Segment && cp.Type == ImNodesLinkControlType_Point) { //Prioritize points
                    smallest_distance = distance;
                    control_primitive_idx_with_smallest_distance = idx;
                    control_primitive_type = cp.Type;
                }
            }
        }
    }

    return control_primitive_idx_with_smallest_distance;
}

ImOptionalIndex ResolveHoveredLabel(const ImObjectPool<ImLabelData>& labels)
{
    for (int idx = 0; idx < labels.Pool.Size; ++idx)
    {
        if (!labels.InUse[idx])
        {
            continue;
        }

        if (labels.Pool[idx].Rect.Contains(GImNodes->MousePos))
            return ImOptionalIndex(idx);
    }
    return ImOptionalIndex();
}

// [SECTION] render helpers

inline ImRect GetItemRect() { return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()); }

inline ImVec2 GetNodeTitleBarOrigin(const ImNodeData& node)
{
    return node.Origin + node.LayoutStyle.Padding;
}

inline ImVec2 GetNodeContentOrigin(const ImNodeData& node)
{
    const ImVec2 title_bar_height =
        ImVec2(0.f, node.TitleBarContentRect.GetHeight() + 2.0f * node.LayoutStyle.Padding.y);
    return node.Origin + title_bar_height + node.LayoutStyle.Padding;
}

inline ImRect GetNodeTitleRect(const ImNodeData& node)
{
    ImRect expanded_title_rect = node.TitleBarContentRect;
    expanded_title_rect.Expand(node.LayoutStyle.Padding);

    return ImRect(
        expanded_title_rect.Min,
        expanded_title_rect.Min + ImVec2(node.Rect.GetWidth(), 0.f) +
            ImVec2(0.f, expanded_title_rect.GetHeight()));
}

void DrawGrid(ImNodesEditorContext& editor, const ImVec2& canvas_size)
{
    const ImVec2 offset = editor.Panning;
    ImU32        line_color = GImNodes->Style.Colors[ImNodesCol_GridLine];
    ImU32        line_color_prim = GImNodes->Style.Colors[ImNodesCol_GridLinePrimary];
    bool         draw_primary = GImNodes->Style.Flags & ImNodesStyleFlags_GridLinesPrimary;

    for (float x = fmodf(offset.x, GImNodes->Style.GridSpacing); x < canvas_size.x;
         x += GImNodes->Style.GridSpacing)
    {
        GImNodes->CanvasDrawList->AddLine(
            EditorSpaceToScreenSpace(ImVec2(x, 0.0f)),
            EditorSpaceToScreenSpace(ImVec2(x, canvas_size.y)),
            offset.x - x == 0.f && draw_primary ? line_color_prim : line_color);
    }

    for (float y = fmodf(offset.y, GImNodes->Style.GridSpacing); y < canvas_size.y;
         y += GImNodes->Style.GridSpacing)
    {
        GImNodes->CanvasDrawList->AddLine(
            EditorSpaceToScreenSpace(ImVec2(0.0f, y)),
            EditorSpaceToScreenSpace(ImVec2(canvas_size.x, y)),
            offset.y - y == 0.f && draw_primary ? line_color_prim : line_color);
    }
}

struct QuadOffsets
{
    ImVec2 TopLeft, BottomLeft, BottomRight, TopRight;
};

QuadOffsets CalculateQuadOffsets(const float side_length)
{
    const float half_side = 0.5f * side_length;

    QuadOffsets offset;

    offset.TopLeft = ImVec2(-half_side, half_side);
    offset.BottomLeft = ImVec2(-half_side, -half_side);
    offset.BottomRight = ImVec2(half_side, -half_side);
    offset.TopRight = ImVec2(half_side, half_side);

    return offset;
}

struct TriangleOffsets
{
    ImVec2 TopLeft, BottomLeft, Right;
};

TriangleOffsets CalculateTriangleOffsets(const float side_length)
{
    // Calculates the Vec2 offsets from an equilateral triangle's midpoint to
    // its vertices. Here is how the left_offset and right_offset are
    // calculated.
    //
    // For an equilateral triangle of side length s, the
    // triangle's height, h, is h = s * sqrt(3) / 2.
    //
    // The length from the base to the midpoint is (1 / 3) * h. The length from
    // the midpoint to the triangle vertex is (2 / 3) * h.
    const float sqrt_3 = sqrtf(3.0f);
    const float left_offset = -0.1666666666667f * sqrt_3 * side_length;
    const float right_offset = 0.333333333333f * sqrt_3 * side_length;
    const float vertical_offset = 0.5f * side_length;

    TriangleOffsets offset;
    offset.TopLeft = ImVec2(left_offset, vertical_offset);
    offset.BottomLeft = ImVec2(left_offset, -vertical_offset);
    offset.Right = ImVec2(right_offset, 0.f);

    return offset;
}

void DrawPinShape(const ImVec2& pin_pos, const ImPinData& pin, const ImU32 pin_color)
{
    static const int CIRCLE_NUM_SEGMENTS = 8;

    switch (pin.Shape)
    {
    case ImNodesPinShape_Circle:
    {
        GImNodes->CanvasDrawList->AddCircle(
            pin_pos,
            GImNodes->Style.PinCircleRadius,
            pin_color,
            CIRCLE_NUM_SEGMENTS,
            GImNodes->Style.PinLineThickness);
    }
    break;
    case ImNodesPinShape_CircleFilled:
    {
        GImNodes->CanvasDrawList->AddCircleFilled(
            pin_pos, GImNodes->Style.PinCircleRadius, pin_color, CIRCLE_NUM_SEGMENTS);
    }
    break;
    case ImNodesPinShape_Quad:
    {
        const QuadOffsets offset = CalculateQuadOffsets(GImNodes->Style.PinQuadSideLength);
        GImNodes->CanvasDrawList->AddQuad(
            pin_pos + offset.TopLeft,
            pin_pos + offset.BottomLeft,
            pin_pos + offset.BottomRight,
            pin_pos + offset.TopRight,
            pin_color,
            GImNodes->Style.PinLineThickness);
    }
    break;
    case ImNodesPinShape_QuadFilled:
    {
        const QuadOffsets offset = CalculateQuadOffsets(GImNodes->Style.PinQuadSideLength);
        GImNodes->CanvasDrawList->AddQuadFilled(
            pin_pos + offset.TopLeft,
            pin_pos + offset.BottomLeft,
            pin_pos + offset.BottomRight,
            pin_pos + offset.TopRight,
            pin_color);
    }
    break;
    case ImNodesPinShape_Triangle:
    {
        const TriangleOffsets offset =
            CalculateTriangleOffsets(GImNodes->Style.PinTriangleSideLength);
        GImNodes->CanvasDrawList->AddTriangle(
            pin_pos + offset.TopLeft,
            pin_pos + offset.BottomLeft,
            pin_pos + offset.Right,
            pin_color,
            // NOTE: for some weird reason, the line drawn by AddTriangle is
            // much thinner than the lines drawn by AddCircle or AddQuad.
            // Multiplying the line thickness by two seemed to solve the
            // problem at a few different thickness values.
            2.f * GImNodes->Style.PinLineThickness);
    }
    break;
    case ImNodesPinShape_TriangleFilled:
    {
        const TriangleOffsets offset =
            CalculateTriangleOffsets(GImNodes->Style.PinTriangleSideLength);
        GImNodes->CanvasDrawList->AddTriangleFilled(
            pin_pos + offset.TopLeft,
            pin_pos + offset.BottomLeft,
            pin_pos + offset.Right,
            pin_color);
    }
    break;
    default:
        IM_ASSERT(!"Invalid PinShape value!");
        break;
    }
}

void DrawPin(ImNodesEditorContext& editor, const int pin_idx)
{
    ImPinData&    pin = editor.Pins.Pool[pin_idx];
    const ImRect& parent_node_rect = editor.Nodes.Pool[pin.ParentNodeIdx].Rect;

    pin.Pos = GetScreenSpacePinCoordinates(parent_node_rect, pin.AttributeRect, pin.Type);

    ImU32 pin_color = pin.ColorStyle.Background;

    if (GImNodes->HoveredPinIdx == pin_idx)
    {
        pin_color = pin.ColorStyle.Hovered;
    }

    DrawPinShape(pin.Pos, pin, pin_color);
}

void DrawNode(ImNodesEditorContext& editor, const int node_idx)
{
    const ImNodeData& node = editor.Nodes.Pool[node_idx];
    ImGui::SetCursorPos(node.Origin + editor.Panning);

    const bool node_hovered =
        GImNodes->HoveredNodeIdx == node_idx &&
        editor.ClickInteraction.Type != ImNodesClickInteractionType_BoxSelection;

    ImU32 node_background = node.ColorStyle.Background;
    ImU32 titlebar_background = node.ColorStyle.Titlebar;

    if (editor.SelectedNodeIndices.contains(node_idx))
    {
        node_background = node.ColorStyle.BackgroundSelected;
        titlebar_background = node.ColorStyle.TitlebarSelected;
    }
    else if (node_hovered)
    {
        node_background = node.ColorStyle.BackgroundHovered;
        titlebar_background = node.ColorStyle.TitlebarHovered;
    }

    {
        // node base
        GImNodes->CanvasDrawList->AddRectFilled(
            node.Rect.Min, node.Rect.Max, node_background, node.LayoutStyle.CornerRounding);

        // title bar:
        if (node.TitleBarContentRect.GetHeight() > 0.f)
        {
            ImRect title_bar_rect = GetNodeTitleRect(node);

#if IMGUI_VERSION_NUM < 18200
            GImNodes->CanvasDrawList->AddRectFilled(
                title_bar_rect.Min,
                title_bar_rect.Max,
                titlebar_background,
                node.LayoutStyle.CornerRounding,
                ImDrawCornerFlags_Top);
#else
            GImNodes->CanvasDrawList->AddRectFilled(
                title_bar_rect.Min,
                title_bar_rect.Max,
                titlebar_background,
                node.LayoutStyle.CornerRounding,
                ImDrawFlags_RoundCornersTop);

#endif
        }

        if ((GImNodes->Style.Flags & ImNodesStyleFlags_NodeOutline) != 0)
        {
#if IMGUI_VERSION_NUM < 18200
            GImNodes->CanvasDrawList->AddRect(
                node.Rect.Min,
                node.Rect.Max,
                node.ColorStyle.Outline,
                node.LayoutStyle.CornerRounding,
                ImDrawCornerFlags_All,
                node.LayoutStyle.BorderThickness);
#else
            GImNodes->CanvasDrawList->AddRect(
                node.Rect.Min,
                node.Rect.Max,
                node.ColorStyle.Outline,
                node.LayoutStyle.CornerRounding,
                ImDrawFlags_RoundCornersAll,
                node.LayoutStyle.BorderThickness);
#endif
        }
    }

    for (int i = 0; i < node.PinIndices.size(); ++i)
    {
        DrawPin(editor, node.PinIndices[i]);
    }

    if (node_hovered)
    {
        GImNodes->HoveredNodeIdx = node_idx;
    }
}

void DrawLabel(ImNodesEditorContext& editor, const int label_idx)
{
    const ImLabelData& label = editor.Labels.Pool[label_idx];
    ImGui::SetCursorPos(label._Origin + editor.Panning);

    const bool label_hovered =
        GImNodes->HoveredLabelIdx == label_idx &&
        editor.ClickInteraction.Type != ImNodesClickInteractionType_BoxSelection;

    ImU32 border_color;

    bool show_border = false;

    if (editor.SelectedLabelIndices.contains(label_idx))
    {
        show_border = true;
        border_color = label.ColorStyle.BorderSelected;
    }
    else if (label_hovered)
    {
        show_border = true;
        border_color = label.ColorStyle.BorderHovered;
    }

    if(show_border)
        GImNodes->CanvasDrawList->AddRect(
            label.Rect.Min, label.Rect.Max, border_color, label.LayoutStyle.CornerRounding);
}

void DrawLink(ImNodesEditorContext& editor, const int link_idx)
{
    const ImLinkData& link = editor.Links.Pool[link_idx];
    const ImPinData&  start_pin = editor.Pins.Pool[link.StartPinIdx];
    const ImPinData&  end_pin = editor.Pins.Pool[link.EndPinIdx];

    const Curve curve = GetCurve(
        start_pin.Pos, end_pin.Pos, start_pin.Type, end_pin.Type, link.LinkType, link.Deformations);

    const bool link_hovered =
        GImNodes->HoveredLinkIdx == link_idx &&
        editor.ClickInteraction.Type != ImNodesClickInteractionType_BoxSelection;

    if (link_hovered)
    {
        GImNodes->HoveredLinkIdx = link_idx;
    }

    // It's possible for a link to be deleted in begin_link_interaction. A user
    // may detach a link, resulting in the link wire snapping to the mouse
    // position.
    //
    // In other words, skip rendering the link if it was deleted.
    if (GImNodes->DeletedLinkIdx == link_idx)
    {
        return;
    }

    ImU32 link_color = link.ColorStyle.Base;
    if (editor.SelectedLinkIndices.contains(link_idx))
    {
        link_color = link.ColorStyle.Selected;
    }
    else if (link_hovered)
    {
        link_color = link.ColorStyle.Hovered;
    }

    DrawCurve(curve, link_color, GImNodes->Style.LinkThickness);
}

void BeginSwappableAttribute() {
    if (GImNodes->CurrentAttributeFlags & ImNodesStyleFlags_AttributeSwappable) {
        ImGui::BeginGroup();
    }
}

void EndSwappableAttribute() {
    if (GImNodes->CurrentAttributeFlags & ImNodesStyleFlags_AttributeSwappable) {
        ImGui::SameLine();
        ImGui::Indent(-0.1);
        ImGui::PushID(ImGui::GetID(GImNodes->CurrentAttributeId));
        ImGui::Selectable("##SwappableAttribute", false, 0, ImGui::GetItemRectSize());
        
        if (ImGui::IsItemActive()) {
            GImNodes->ActiveSwappableAttribute = true;
            GImNodes->ActiveSwappableAttributeId = GImNodes->CurrentAttributeId;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
            GImNodes->HoveredSwappableAttribute = true;
            GImNodes->HoveredSwappableAttributeId = GImNodes->CurrentAttributeId;
        }

        ImGui::PopID();
        ImGui::EndGroup();
    }
}

void BeginPinAttribute(
    const int                  id,
    const ImNodesAttributeType type,
    const ImNodesPinShape      shape,
    const int                  node_idx)
{
    // Make sure to call BeginNode() before calling
    // BeginAttribute()
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_Node);
    GImNodes->CurrentScope = ImNodesScope_Attribute;

    BeginSwappableAttribute();

    ImGui::BeginGroup();
    ImGui::PushID(id);

    ImNodesEditorContext& editor = EditorContextGet();

    GImNodes->CurrentAttributeId = id;

    const int pin_idx = ObjectPoolFindOrCreateIndex(editor.Pins, id);
    GImNodes->CurrentPinIdx = pin_idx;
    ImPinData& pin = editor.Pins.Pool[pin_idx];
    pin.Id = id;
    pin.ParentNodeIdx = node_idx;
    pin.Type = type;
    pin.Shape = shape;
    pin.Flags = GImNodes->CurrentAttributeFlags;
    pin.ColorStyle.Background = GImNodes->Style.Colors[ImNodesCol_Pin];
    pin.ColorStyle.Hovered = GImNodes->Style.Colors[ImNodesCol_PinHovered];
}

void EndPinAttribute()
{
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_Attribute);
    GImNodes->CurrentScope = ImNodesScope_Node;

    ImGui::PopID();
    ImGui::EndGroup();

    if (ImGui::IsItemActive())
    {
        GImNodes->ActiveAttribute = true;
        GImNodes->ActiveAttributeId = GImNodes->CurrentAttributeId;
    }

    ImNodesEditorContext& editor = EditorContextGet();
    ImPinData&            pin = editor.Pins.Pool[GImNodes->CurrentPinIdx];
    ImNodeData&           node = editor.Nodes.Pool[GImNodes->CurrentNodeIdx];
    pin.AttributeRect = GetItemRect();
    node.PinIndices.push_back(GImNodes->CurrentPinIdx);

    EndSwappableAttribute();
}

void Initialize(ImNodesContext* context)
{
    context->CanvasOriginScreenSpace = ImVec2(0.0f, 0.0f);
    context->CanvasRectScreenSpace = ImRect(ImVec2(0.f, 0.f), ImVec2(0.f, 0.f));
    context->CurrentScope = ImNodesScope_None;

    context->CurrentPinIdx = INT_MAX;
    context->CurrentNodeIdx = INT_MAX;
    context->CurrentLabelIdx = INT_MAX;

    context->DefaultEditorCtx = EditorContextCreate();
    context->EditorCtx = context->DefaultEditorCtx;

    context->CurrentAttributeFlags = ImNodesAttributeFlags_None;
    context->AttributeFlagStack.push_back(GImNodes->CurrentAttributeFlags);

    StyleColorsDark(&context->Style);
}

void Shutdown(ImNodesContext* ctx) { EditorContextFree(ctx->DefaultEditorCtx); }

// [SECTION] minimap

static inline bool IsMiniMapActive()
{
    ImNodesEditorContext& editor = EditorContextGet();
    return editor.MiniMapEnabled && editor.MiniMapSizeFraction > 0.0f;
}

static inline bool IsMiniMapHovered()
{
    ImNodesEditorContext& editor = EditorContextGet();
    return IsMiniMapActive() &&
           ImGui::IsMouseHoveringRect(
               editor.MiniMapRectScreenSpace.Min, editor.MiniMapRectScreenSpace.Max);
}

static inline void CalcMiniMapLayout()
{
    ImNodesEditorContext& editor = EditorContextGet();
    const ImVec2          offset = GImNodes->Style.MiniMapOffset;
    const ImVec2          border = GImNodes->Style.MiniMapPadding;
    const ImRect          editor_rect = GImNodes->CanvasRectScreenSpace;

    // Compute the size of the mini-map area
    ImVec2 mini_map_size;
    float  mini_map_scaling;
    {
        const ImVec2 max_size =
            ImFloor(editor_rect.GetSize() * editor.MiniMapSizeFraction - border * 2.0f);
        const float  max_size_aspect_ratio = max_size.x / max_size.y;
        const ImVec2 grid_content_size = editor.GridContentBounds.IsInverted()
                                             ? max_size
                                             : ImFloor(editor.GridContentBounds.GetSize());
        const float  grid_content_aspect_ratio = grid_content_size.x / grid_content_size.y;
        mini_map_size = ImFloor(
            grid_content_aspect_ratio > max_size_aspect_ratio
                ? ImVec2(max_size.x, max_size.x / grid_content_aspect_ratio)
                : ImVec2(max_size.y * grid_content_aspect_ratio, max_size.y));
        mini_map_scaling = mini_map_size.x / grid_content_size.x;
    }

    // Compute location of the mini-map
    ImVec2 mini_map_pos;
    {
        ImVec2 align;

        switch (editor.MiniMapLocation)
        {
        case ImNodesMiniMapLocation_BottomRight:
            align.x = 1.0f;
            align.y = 1.0f;
            break;
        case ImNodesMiniMapLocation_BottomLeft:
            align.x = 0.0f;
            align.y = 1.0f;
            break;
        case ImNodesMiniMapLocation_TopRight:
            align.x = 1.0f;
            align.y = 0.0f;
            break;
        case ImNodesMiniMapLocation_TopLeft: // [[fallthrough]]
        default:
            align.x = 0.0f;
            align.y = 0.0f;
            break;
        }

        const ImVec2 top_left_pos = editor_rect.Min + offset + border;
        const ImVec2 bottom_right_pos = editor_rect.Max - offset - border - mini_map_size;
        mini_map_pos = ImFloor(ImLerp(top_left_pos, bottom_right_pos, align));
    }

    editor.MiniMapRectScreenSpace =
        ImRect(mini_map_pos - border, mini_map_pos + mini_map_size + border);
    editor.MiniMapContentScreenSpace = ImRect(mini_map_pos, mini_map_pos + mini_map_size);
    editor.MiniMapScaling = mini_map_scaling;
}

static void MiniMapDrawNode(ImNodesEditorContext& editor, const int node_idx)
{
    const ImNodeData& node = editor.Nodes.Pool[node_idx];

    const ImRect node_rect = ScreenSpaceToMiniMapSpace(editor, node.Rect);

    // Round to near whole pixel value for corner-rounding to prevent visual glitches
    const float mini_map_node_rounding =
        floorf(node.LayoutStyle.CornerRounding * editor.MiniMapScaling);

    ImU32 mini_map_node_background;

    if (editor.ClickInteraction.Type == ImNodesClickInteractionType_None &&
        ImGui::IsMouseHoveringRect(node_rect.Min, node_rect.Max))
    {
        mini_map_node_background = GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundHovered];

        // Run user callback when hovering a mini-map node
        if (editor.MiniMapNodeHoveringCallback)
        {
            editor.MiniMapNodeHoveringCallback(node.Id, editor.MiniMapNodeHoveringCallbackUserData);
        }
    }
    else if (editor.SelectedNodeIndices.contains(node_idx))
    {
        mini_map_node_background = GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundSelected];
    }
    else
    {
        mini_map_node_background = GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackground];
    }

    const ImU32 mini_map_node_outline = GImNodes->Style.Colors[ImNodesCol_MiniMapNodeOutline];

    GImNodes->CanvasDrawList->AddRectFilled(
        node_rect.Min, node_rect.Max, mini_map_node_background, mini_map_node_rounding);

    GImNodes->CanvasDrawList->AddRect(
        node_rect.Min, node_rect.Max, mini_map_node_outline, mini_map_node_rounding);
}

static void MiniMapDrawLink(ImNodesEditorContext& editor, const int link_idx)
{
    const ImLinkData& link = editor.Links.Pool[link_idx];
    const ImPinData&  start_pin = editor.Pins.Pool[link.StartPinIdx];
    const ImPinData&  end_pin = editor.Pins.Pool[link.EndPinIdx];

    const Curve curve = GetCurve(
        ScreenSpaceToMiniMapSpace(editor, start_pin.Pos),
        ScreenSpaceToMiniMapSpace(editor, end_pin.Pos),
        start_pin.Type,
        end_pin.Type,
        GImNodes->Style.LinkLineSegmentsPerLength / editor.MiniMapScaling,
        link.LinkType, link.Deformations);

    // It's possible for a link to be deleted in begin_link_interaction. A user
    // may detach a link, resulting in the link wire snapping to the mouse
    // position.
    //
    // In other words, skip rendering the link if it was deleted.
    if (GImNodes->DeletedLinkIdx == link_idx)
    {
        return;
    }

    const ImU32 link_color =
        GImNodes->Style.Colors
            [editor.SelectedLinkIndices.contains(link_idx) ? ImNodesCol_MiniMapLinkSelected
                                                           : ImNodesCol_MiniMapLink];

    DrawCurve(curve, link_color, GImNodes->Style.LinkThickness * editor.MiniMapScaling);
}

static void MiniMapUpdate()
{
    ImNodesEditorContext& editor = EditorContextGet();

    ImU32 mini_map_background;

    if (IsMiniMapHovered())
    {
        mini_map_background = GImNodes->Style.Colors[ImNodesCol_MiniMapBackgroundHovered];
    }
    else
    {
        mini_map_background = GImNodes->Style.Colors[ImNodesCol_MiniMapBackground];
    }

    // Create a child window bellow mini-map, so it blocks all mouse interaction on canvas.
    int flags = ImGuiWindowFlags_NoBackground;
    ImGui::SetCursorScreenPos(editor.MiniMapRectScreenSpace.Min);
    ImGui::BeginChild("minimap", editor.MiniMapRectScreenSpace.GetSize(), false, flags);

    const ImRect& mini_map_rect = editor.MiniMapRectScreenSpace;

    // Draw minimap background and border
    GImNodes->CanvasDrawList->AddRectFilled(
        mini_map_rect.Min, mini_map_rect.Max, mini_map_background);

    GImNodes->CanvasDrawList->AddRect(
        mini_map_rect.Min, mini_map_rect.Max, GImNodes->Style.Colors[ImNodesCol_MiniMapOutline]);

    // Clip draw list items to mini-map rect (after drawing background/outline)
    GImNodes->CanvasDrawList->PushClipRect(
        mini_map_rect.Min, mini_map_rect.Max, true /* intersect with editor clip-rect */);

    // Draw links first so they appear under nodes, and we can use the same draw channel
    for (int link_idx = 0; link_idx < editor.Links.Pool.size(); ++link_idx)
    {
        if (editor.Links.InUse[link_idx])
        {
            MiniMapDrawLink(editor, link_idx);
        }
    }

    for (int node_idx = 0; node_idx < editor.Nodes.Pool.size(); ++node_idx)
    {
        if (editor.Nodes.InUse[node_idx])
        {
            MiniMapDrawNode(editor, node_idx);
        }
    }

    // Draw editor canvas rect inside mini-map
    {
        const ImU32  canvas_color = GImNodes->Style.Colors[ImNodesCol_MiniMapCanvas];
        const ImU32  outline_color = GImNodes->Style.Colors[ImNodesCol_MiniMapCanvasOutline];
        const ImRect rect = ScreenSpaceToMiniMapSpace(editor, GImNodes->CanvasRectScreenSpace);

        GImNodes->CanvasDrawList->AddRectFilled(rect.Min, rect.Max, canvas_color);
        GImNodes->CanvasDrawList->AddRect(rect.Min, rect.Max, outline_color);
    }

    // Have to pop mini-map clip rect
    GImNodes->CanvasDrawList->PopClipRect();

    bool mini_map_is_hovered = ImGui::IsWindowHovered();

    ImGui::EndChild();

    bool center_on_click = mini_map_is_hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
                           editor.ClickInteraction.Type == ImNodesClickInteractionType_None &&
                           !GImNodes->NodeIdxSubmissionOrder.empty();
    if (center_on_click)
    {
        ImVec2 target = MiniMapSpaceToGridSpace(editor, ImGui::GetMousePos());
        ImVec2 center = GImNodes->CanvasRectScreenSpace.GetSize() * 0.5f;
        editor.Panning = ImFloor(center - target);
    }

    // Reset callback info after use
    editor.MiniMapNodeHoveringCallback = NULL;
    editor.MiniMapNodeHoveringCallbackUserData = NULL;
}

// [SECTION] selection helpers

template<typename T>
void SelectObject(const ImObjectPool<T>& objects, ImVector<int>& selected_indices, const int id)
{
    const int idx = ObjectPoolFind(objects, id);
    IM_ASSERT(idx >= 0);
    IM_ASSERT(selected_indices.find(idx) == selected_indices.end());
    selected_indices.push_back(idx);
}

template<typename T>
void ClearObjectSelection(
    const ImObjectPool<T>& objects,
    ImVector<int>&         selected_indices,
    const int              id)
{
    const int idx = ObjectPoolFind(objects, id);
    IM_ASSERT(idx >= 0);
    IM_ASSERT(selected_indices.find(idx) != selected_indices.end());
    selected_indices.find_erase_unsorted(idx);
}

template<typename T>
bool IsObjectSelected(const ImObjectPool<T>& objects, ImVector<int>& selected_indices, const int id)
{
    const int idx = ObjectPoolFind(objects, id);
    return selected_indices.find(idx) != selected_indices.end();
}

} // namespace
} // namespace IMNODES_NAMESPACE

// [SECTION] API implementation

ImNodesIO::EmulateThreeButtonMouse::EmulateThreeButtonMouse() : Modifier(NULL) {}

ImNodesIO::LinkDetachWithModifierClick::LinkDetachWithModifierClick() : Modifier(NULL) {}

ImNodesIO::MultipleSelectModifier::MultipleSelectModifier() : Modifier(NULL) {}

ImNodesIO::ImNodesIO()
    : EmulateThreeButtonMouse(), LinkDetachWithModifierClick(),
      AltMouseButton(ImGuiMouseButton_Middle), AutoPanningSpeed(1000.0f)
{
}

ImNodesStyle::ImNodesStyle()
    : GridSpacing(24.f), NodeCornerRounding(4.f), NodePadding(8.f, 16.f), NodeBorderThickness(1.f),
      LinkThickness(3.f), LinkLineSegmentsPerLength(0.1f), LinkHoverDistance(10.f), LinkSlopedMinSlope(5.0f), LinkSlopedMinOffset(50.0f), LinkCreationType(ImNodesLinkType_::ImNodesLinkType_Bezier), LabelPadding(3.f, 3.f), LabelCornerRounding(3.f), LabelDraggable(true),
      PinCircleRadius(4.f), PinQuadSideLength(7.f), PinTriangleSideLength(9.5),
      PinLineThickness(1.f), PinHoverRadius(10.f), PinOffset(0.f), MiniMapPadding(8.0f, 8.0f),
      MiniMapOffset(4.0f, 4.0f), Flags(ImNodesStyleFlags_NodeOutline | ImNodesStyleFlags_GridLines),
      Colors()
{
}

namespace IMNODES_NAMESPACE
{
ImNodesContext* CreateContext()
{
    ImNodesContext* ctx = IM_NEW(ImNodesContext)();
    if (GImNodes == NULL)
        SetCurrentContext(ctx);
    Initialize(ctx);
    return ctx;
}

void DestroyContext(ImNodesContext* ctx)
{
    if (ctx == NULL)
        ctx = GImNodes;
    Shutdown(ctx);
    if (GImNodes == ctx)
        SetCurrentContext(NULL);
    IM_DELETE(ctx);
}

ImNodesContext* GetCurrentContext() { return GImNodes; }

void SetCurrentContext(ImNodesContext* ctx) { GImNodes = ctx; }

ImNodesEditorContext* EditorContextCreate()
{
    void* mem = ImGui::MemAlloc(sizeof(ImNodesEditorContext));
    new (mem) ImNodesEditorContext();
    return (ImNodesEditorContext*)mem;
}

void EditorContextFree(ImNodesEditorContext* ctx)
{
    ctx->~ImNodesEditorContext();
    ImGui::MemFree(ctx);
}

void EditorContextSet(ImNodesEditorContext* ctx) { GImNodes->EditorCtx = ctx; }

ImVec2 EditorContextGetPanning()
{
    const ImNodesEditorContext& editor = EditorContextGet();
    return editor.Panning;
}

void EditorContextResetPanning(const ImVec2& pos)
{
    ImNodesEditorContext& editor = EditorContextGet();
    editor.Panning = pos;
}

void EditorContextMoveToNode(const int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    ImNodeData&           node = ObjectPoolFindOrCreateObject(editor.Nodes, node_id);

    editor.Panning.x = -node.Origin.x;
    editor.Panning.y = -node.Origin.y;
}

void SetImGuiContext(ImGuiContext* ctx) { ImGui::SetCurrentContext(ctx); }

ImNodesIO& GetIO() { return GImNodes->Io; }

ImNodesStyle& GetStyle() { return GImNodes->Style; }

void StyleColorsDark(ImNodesStyle* dest)
{
    if (dest == nullptr)
    {
        dest = &GImNodes->Style;
    }

    dest->Colors[ImNodesCol_NodeBackground] = IM_COL32(50, 50, 50, 255);
    dest->Colors[ImNodesCol_NodeBackgroundHovered] = IM_COL32(75, 75, 75, 255);
    dest->Colors[ImNodesCol_NodeBackgroundSelected] = IM_COL32(75, 75, 75, 255);
    dest->Colors[ImNodesCol_NodeOutline] = IM_COL32(100, 100, 100, 255);
    // title bar colors match ImGui's titlebg colors
    dest->Colors[ImNodesCol_TitleBar] = IM_COL32(41, 74, 122, 255);
    dest->Colors[ImNodesCol_TitleBarHovered] = IM_COL32(66, 150, 250, 255);
    dest->Colors[ImNodesCol_TitleBarSelected] = IM_COL32(66, 150, 250, 255);
    // link colors match ImGui's slider grab colors
    dest->Colors[ImNodesCol_Link] = IM_COL32(61, 133, 224, 200);
    dest->Colors[ImNodesCol_LinkHovered] = IM_COL32(66, 150, 250, 255);
    dest->Colors[ImNodesCol_LinkSelected] = IM_COL32(66, 150, 250, 255);
    dest->Colors[ImNodesCol_LabelHovered] = dest->Colors[ImNodesCol_LinkHovered];
    dest->Colors[ImNodesCol_LabelSelected] = dest->Colors[ImNodesCol_LinkSelected];
    // pin colors match ImGui's button colors
    dest->Colors[ImNodesCol_Pin] = IM_COL32(53, 150, 250, 180);
    dest->Colors[ImNodesCol_PinHovered] = IM_COL32(53, 150, 250, 255);

    dest->Colors[ImNodesCol_BoxSelector] = IM_COL32(61, 133, 224, 30);
    dest->Colors[ImNodesCol_BoxSelectorOutline] = IM_COL32(61, 133, 224, 150);

    dest->Colors[ImNodesCol_GridBackground] = IM_COL32(40, 40, 50, 200);
    dest->Colors[ImNodesCol_GridLine] = IM_COL32(200, 200, 200, 40);
    dest->Colors[ImNodesCol_GridLinePrimary] = IM_COL32(240, 240, 240, 60);

    // minimap colors
    dest->Colors[ImNodesCol_MiniMapBackground] = IM_COL32(25, 25, 25, 150);
    dest->Colors[ImNodesCol_MiniMapBackgroundHovered] = IM_COL32(25, 25, 25, 200);
    dest->Colors[ImNodesCol_MiniMapOutline] = IM_COL32(150, 150, 150, 100);
    dest->Colors[ImNodesCol_MiniMapOutlineHovered] = IM_COL32(150, 150, 150, 200);
    dest->Colors[ImNodesCol_MiniMapNodeBackground] = IM_COL32(200, 200, 200, 100);
    dest->Colors[ImNodesCol_MiniMapNodeBackgroundHovered] = IM_COL32(200, 200, 200, 255);
    dest->Colors[ImNodesCol_MiniMapNodeBackgroundSelected] =
        dest->Colors[ImNodesCol_MiniMapNodeBackgroundHovered];
    dest->Colors[ImNodesCol_MiniMapNodeOutline] = IM_COL32(200, 200, 200, 100);
    dest->Colors[ImNodesCol_MiniMapLink] = dest->Colors[ImNodesCol_Link];
    dest->Colors[ImNodesCol_MiniMapLinkSelected] = dest->Colors[ImNodesCol_LinkSelected];
    dest->Colors[ImNodesCol_MiniMapCanvas] = IM_COL32(200, 200, 200, 25);
    dest->Colors[ImNodesCol_MiniMapCanvasOutline] = IM_COL32(200, 200, 200, 200);
}

void StyleColorsClassic(ImNodesStyle* dest)
{
    if (dest == nullptr)
    {
        dest = &GImNodes->Style;
    }

    dest->Colors[ImNodesCol_NodeBackground] = IM_COL32(50, 50, 50, 255);
    dest->Colors[ImNodesCol_NodeBackgroundHovered] = IM_COL32(75, 75, 75, 255);
    dest->Colors[ImNodesCol_NodeBackgroundSelected] = IM_COL32(75, 75, 75, 255);
    dest->Colors[ImNodesCol_NodeOutline] = IM_COL32(100, 100, 100, 255);
    dest->Colors[ImNodesCol_TitleBar] = IM_COL32(69, 69, 138, 255);
    dest->Colors[ImNodesCol_TitleBarHovered] = IM_COL32(82, 82, 161, 255);
    dest->Colors[ImNodesCol_TitleBarSelected] = IM_COL32(82, 82, 161, 255);
    dest->Colors[ImNodesCol_Link] = IM_COL32(255, 255, 255, 100);
    dest->Colors[ImNodesCol_LinkHovered] = IM_COL32(105, 99, 204, 153);
    dest->Colors[ImNodesCol_LinkSelected] = IM_COL32(105, 99, 204, 153);
    dest->Colors[ImNodesCol_LabelHovered] = dest->Colors[ImNodesCol_LinkHovered];
    dest->Colors[ImNodesCol_LabelSelected] = dest->Colors[ImNodesCol_LinkSelected];
    dest->Colors[ImNodesCol_Pin] = IM_COL32(89, 102, 156, 170);
    dest->Colors[ImNodesCol_PinHovered] = IM_COL32(102, 122, 179, 200);
    dest->Colors[ImNodesCol_BoxSelector] = IM_COL32(82, 82, 161, 100);
    dest->Colors[ImNodesCol_BoxSelectorOutline] = IM_COL32(82, 82, 161, 255);
    dest->Colors[ImNodesCol_GridBackground] = IM_COL32(40, 40, 50, 200);
    dest->Colors[ImNodesCol_GridLine] = IM_COL32(200, 200, 200, 40);
    dest->Colors[ImNodesCol_GridLinePrimary] = IM_COL32(240, 240, 240, 60);

    // minimap colors
    dest->Colors[ImNodesCol_MiniMapBackground] = IM_COL32(25, 25, 25, 100);
    dest->Colors[ImNodesCol_MiniMapBackgroundHovered] = IM_COL32(25, 25, 25, 200);
    dest->Colors[ImNodesCol_MiniMapOutline] = IM_COL32(150, 150, 150, 100);
    dest->Colors[ImNodesCol_MiniMapOutlineHovered] = IM_COL32(150, 150, 150, 200);
    dest->Colors[ImNodesCol_MiniMapNodeBackground] = IM_COL32(200, 200, 200, 100);
    dest->Colors[ImNodesCol_MiniMapNodeBackgroundSelected] =
        dest->Colors[ImNodesCol_MiniMapNodeBackgroundHovered];
    dest->Colors[ImNodesCol_MiniMapNodeBackgroundSelected] = IM_COL32(200, 200, 240, 255);
    dest->Colors[ImNodesCol_MiniMapNodeOutline] = IM_COL32(200, 200, 200, 100);
    dest->Colors[ImNodesCol_MiniMapLink] = dest->Colors[ImNodesCol_Link];
    dest->Colors[ImNodesCol_MiniMapLinkSelected] = dest->Colors[ImNodesCol_LinkSelected];
    dest->Colors[ImNodesCol_MiniMapCanvas] = IM_COL32(200, 200, 200, 25);
    dest->Colors[ImNodesCol_MiniMapCanvasOutline] = IM_COL32(200, 200, 200, 200);
}

void StyleColorsLight(ImNodesStyle* dest)
{
    if (dest == nullptr)
    {
        dest = &GImNodes->Style;
    }

    dest->Colors[ImNodesCol_NodeBackground] = IM_COL32(240, 240, 240, 255);
    dest->Colors[ImNodesCol_NodeBackgroundHovered] = IM_COL32(240, 240, 240, 255);
    dest->Colors[ImNodesCol_NodeBackgroundSelected] = IM_COL32(240, 240, 240, 255);
    dest->Colors[ImNodesCol_NodeOutline] = IM_COL32(100, 100, 100, 255);
    dest->Colors[ImNodesCol_TitleBar] = IM_COL32(248, 248, 248, 255);
    dest->Colors[ImNodesCol_TitleBarHovered] = IM_COL32(209, 209, 209, 255);
    dest->Colors[ImNodesCol_TitleBarSelected] = IM_COL32(209, 209, 209, 255);
    // original imgui values: 66, 150, 250
    dest->Colors[ImNodesCol_Link] = IM_COL32(66, 150, 250, 100);
    // original imgui values: 117, 138, 204
    dest->Colors[ImNodesCol_LinkHovered] = IM_COL32(66, 150, 250, 242);
    dest->Colors[ImNodesCol_LinkSelected] = IM_COL32(66, 150, 250, 242);
    dest->Colors[ImNodesCol_LabelHovered] = dest->Colors[ImNodesCol_LinkHovered];
    dest->Colors[ImNodesCol_LabelSelected] = dest->Colors[ImNodesCol_LinkSelected];
    // original imgui values: 66, 150, 250
    dest->Colors[ImNodesCol_Pin] = IM_COL32(66, 150, 250, 160);
    dest->Colors[ImNodesCol_PinHovered] = IM_COL32(66, 150, 250, 255);
    dest->Colors[ImNodesCol_BoxSelector] = IM_COL32(90, 170, 250, 30);
    dest->Colors[ImNodesCol_BoxSelectorOutline] = IM_COL32(90, 170, 250, 150);
    dest->Colors[ImNodesCol_GridBackground] = IM_COL32(225, 225, 225, 255);
    dest->Colors[ImNodesCol_GridLine] = IM_COL32(180, 180, 180, 100);
    dest->Colors[ImNodesCol_GridLinePrimary] = IM_COL32(120, 120, 120, 100);

    // minimap colors
    dest->Colors[ImNodesCol_MiniMapBackground] = IM_COL32(25, 25, 25, 100);
    dest->Colors[ImNodesCol_MiniMapBackgroundHovered] = IM_COL32(25, 25, 25, 200);
    dest->Colors[ImNodesCol_MiniMapOutline] = IM_COL32(150, 150, 150, 100);
    dest->Colors[ImNodesCol_MiniMapOutlineHovered] = IM_COL32(150, 150, 150, 200);
    dest->Colors[ImNodesCol_MiniMapNodeBackground] = IM_COL32(200, 200, 200, 100);
    dest->Colors[ImNodesCol_MiniMapNodeBackgroundSelected] =
        dest->Colors[ImNodesCol_MiniMapNodeBackgroundHovered];
    dest->Colors[ImNodesCol_MiniMapNodeBackgroundSelected] = IM_COL32(200, 200, 240, 255);
    dest->Colors[ImNodesCol_MiniMapNodeOutline] = IM_COL32(200, 200, 200, 100);
    dest->Colors[ImNodesCol_MiniMapLink] = dest->Colors[ImNodesCol_Link];
    dest->Colors[ImNodesCol_MiniMapLinkSelected] = dest->Colors[ImNodesCol_LinkSelected];
    dest->Colors[ImNodesCol_MiniMapCanvas] = IM_COL32(200, 200, 200, 25);
    dest->Colors[ImNodesCol_MiniMapCanvasOutline] = IM_COL32(200, 200, 200, 200);
}

void StyleColorsBluePrint(ImNodesStyle* dest)
{
    if (dest == nullptr)
    {
        dest = &GImNodes->Style;
    }

    dest->LinkCreationType = ImNodesLinkType_Sloped;
    dest->NodeBorderThickness = 2.0f;
    dest->Flags |= ImNodesStyleFlags_AttributeSwappable | ImNodesStyleFlags_GridSnapping;

    dest->Colors[ImNodesCol_GridBackground] = IM_COL32(5, 69, 141, 255);
    dest->Colors[ImNodesCol_GridLine] = IM_COL32(32, 109, 177, 255);
    dest->Colors[ImNodesCol_Link] = IM_COL32(200, 200, 200, 255);
    dest->Colors[ImNodesCol_LinkHovered] = IM_COL32(255, 255, 255, 255);
    dest->Colors[ImNodesCol_LinkSelected] = IM_COL32(255, 255, 255, 255);
    dest->Colors[ImNodesCol_LabelHovered] = dest->Colors[ImNodesCol_LinkHovered];
    dest->Colors[ImNodesCol_LabelSelected] = dest->Colors[ImNodesCol_LinkSelected];
    dest->Colors[ImNodesCol_NodeOutline] = IM_COL32(200, 200, 200, 255);

    dest->Colors[ImNodesCol_NodeBackground] = IM_COL32(5, 69, 141, 100);
    dest->Colors[ImNodesCol_NodeBackgroundHovered] = IM_COL32(25, 89, 161, 150);
    dest->Colors[ImNodesCol_NodeBackgroundSelected] = IM_COL32(25, 89, 161, 150);

    dest->Colors[ImNodesCol_TitleBar] = IM_COL32(25, 89, 161, 0);
    dest->Colors[ImNodesCol_TitleBarHovered] = IM_COL32(45, 109, 181, 0);
    dest->Colors[ImNodesCol_TitleBarSelected] = IM_COL32(45, 109, 181, 0);

    dest->Colors[ImNodesCol_Pin] = IM_COL32(200, 200, 200, 255);
    dest->Colors[ImNodesCol_PinHovered] = IM_COL32(255, 255, 255, 255);
    dest->Colors[ImNodesCol_BoxSelector] = IM_COL32(250, 250, 250, 20);
    dest->Colors[ImNodesCol_BoxSelectorOutline] = IM_COL32(255, 255, 255, 255);

}

void BeginNodeEditor()
{
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_None);
    GImNodes->CurrentScope = ImNodesScope_Editor;

    // Reset state from previous pass

    ImNodesEditorContext& editor = EditorContextGet();
    editor.AutoPanningDelta = ImVec2(0, 0);
    editor.GridContentBounds = ImRect(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
    editor.MiniMapEnabled = false;
    ObjectPoolReset(editor.Nodes);
    ObjectPoolReset(editor.Pins);
    ObjectPoolReset(editor.Links);
    ObjectPoolReset(editor.LinkControls);
    ObjectPoolReset(editor.Labels);

    GImNodes->HoveredNodeIdx.Reset();
    GImNodes->HoveredLinkIdx.Reset();
    GImNodes->HoveredLinkControlIdx.Reset();
    GImNodes->HoveredPinIdx.Reset();
    GImNodes->HoveredLabelIdx.Reset();
    GImNodes->DeletedLinkIdx.Reset();
    GImNodes->SnapLinkIdx.Reset();

    GImNodes->PopedEvent.Reset();
    GImNodes->UnpopedEvent.Reset();

    GImNodes->NodeIndicesOverlappingWithMouse.clear();

    GImNodes->ImNodesUIState = ImNodesUIState_None;

    GImNodes->MousePos = ImGui::GetIO().MousePos;
    GImNodes->LeftMouseClicked = ImGui::IsMouseClicked(0);
    GImNodes->LeftMouseReleased = ImGui::IsMouseReleased(0);
    GImNodes->LeftMouseDragging = ImGui::IsMouseDragging(0, 0.0f);
    GImNodes->AltMouseClicked =
        (GImNodes->Io.EmulateThreeButtonMouse.Modifier != NULL &&
         *GImNodes->Io.EmulateThreeButtonMouse.Modifier && GImNodes->LeftMouseClicked) ||
        ImGui::IsMouseClicked(GImNodes->Io.AltMouseButton);
    GImNodes->AltMouseDragging =
        (GImNodes->Io.EmulateThreeButtonMouse.Modifier != NULL && GImNodes->LeftMouseDragging &&
         (*GImNodes->Io.EmulateThreeButtonMouse.Modifier)) ||
        ImGui::IsMouseDragging(GImNodes->Io.AltMouseButton, 0.0f);
    GImNodes->AltMouseScrollDelta = ImGui::GetIO().MouseWheel;
    GImNodes->MultipleSelectModifier =
        (GImNodes->Io.MultipleSelectModifier.Modifier != NULL
             ? *GImNodes->Io.MultipleSelectModifier.Modifier
             : ImGui::GetIO().KeyCtrl);

    GImNodes->ActiveAttribute = false;
    GImNodes->ActiveSwappableAttribute = false;
    GImNodes->HoveredSwappableAttribute = false;

    ImGui::BeginGroup();
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, GImNodes->Style.Colors[ImNodesCol_GridBackground]);
        ImGui::BeginChild(
            "scrolling_region",
            ImVec2(0.f, 0.f),
            true,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollWithMouse);
        GImNodes->CanvasOriginScreenSpace = ImGui::GetCursorScreenPos();

        // NOTE: we have to fetch the canvas draw list *after* we call
        // BeginChild(), otherwise the ImGui UI elements are going to be
        // rendered into the parent window draw list.
        DrawListSet(ImGui::GetWindowDrawList());

        {
            const ImVec2 canvas_size = ImGui::GetWindowSize();
            GImNodes->CanvasRectScreenSpace = ImRect(
                EditorSpaceToScreenSpace(ImVec2(0.f, 0.f)), EditorSpaceToScreenSpace(canvas_size));

            if (GImNodes->Style.Flags & ImNodesStyleFlags_GridLines)
            {
                DrawGrid(editor, canvas_size);
            }
        }
    }
}
//descriptor of a function delcared after, might change some stuff to avoid it...
void LinkControl(ImNodesEditorContext& editor, int link_idx);
void EndNodeEditor()
{
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_Editor);
    GImNodes->CurrentScope = ImNodesScope_None;

    ImNodesEditorContext& editor = EditorContextGet();

    //resolve visible control primitive
    for(int link_idx : editor.SelectedLinkIndices) 
        LinkControl(editor, link_idx);

    bool no_grid_content = editor.GridContentBounds.IsInverted();
    if (no_grid_content)
    {
        editor.GridContentBounds = ScreenSpaceToGridSpace(editor, GImNodes->CanvasRectScreenSpace);
    }

    // Detect ImGui interaction first, because it blocks interaction with the rest of the UI

    if (GImNodes->LeftMouseClicked && ImGui::IsAnyItemActive())
    {
        editor.ClickInteraction.Type = ImNodesClickInteractionType_ImGuiItem;
    }

    // Detect which UI element is being hovered over. Detection is done in a hierarchical fashion,
    // because a UI element being hovered excludes any other as being hovered over.

    // Don't do hovering detection for nodes/links/pins when interacting with the mini-map, since
    // its an *overlay* with its own interaction behavior and must have precedence during mouse
    // interaction.

    if ((editor.ClickInteraction.Type == ImNodesClickInteractionType_None ||
         editor.ClickInteraction.Type == ImNodesClickInteractionType_LinkCreation) &&
        MouseInCanvas() && !IsMiniMapHovered())
    {
        // Pins needs some special care. We need to check the depth stack to see which pins are
        // being occluded by other nodes.
        ResolveOccludedPins(editor, GImNodes->OccludedPinIndices);

        GImNodes->HoveredPinIdx = ResolveHoveredPin(editor.Pins, GImNodes->OccludedPinIndices);

        if (!GImNodes->HoveredPinIdx.HasValue())
        {
            // Resolve which node is actually on top and being hovered using the depth stack.
            GImNodes->HoveredNodeIdx = ResolveHoveredNode(editor.NodeDepthOrder);
        }

        if (!GImNodes->HoveredPinIdx.HasValue() && !GImNodes->HoveredNodeIdx.HasValue())
        {
            // Resolve which node is actually on top and being hovered using the depth stack.
            GImNodes->HoveredLinkControlIdx = ResolveHoveredLinkControl(editor.LinkControls, editor.Links, editor.Pins);
        }

        if (!GImNodes->HoveredPinIdx.HasValue() && !GImNodes->HoveredNodeIdx.HasValue() && !GImNodes->HoveredLinkControlIdx.HasValue())
        {
            // Resolve which node is actually on top and being hovered using the depth stack.
            GImNodes->HoveredLabelIdx = ResolveHoveredLabel(editor.Labels);
        }

        // We don't check for hovered pins here, because if we want to detach a link by clicking and
        // dragging, we need to have both a link and pin hovered.
        if (!GImNodes->HoveredPinIdx.HasValue() && !GImNodes->HoveredNodeIdx.HasValue() && !GImNodes->HoveredLinkControlIdx.HasValue() && !GImNodes->HoveredLabelIdx.HasValue())
        {
            GImNodes->HoveredLinkIdx = ResolveHoveredLink(editor.Links, editor.Pins);
        }
    }

    for (int node_idx = 0; node_idx < editor.Nodes.Pool.size(); ++node_idx)
    {
        if (editor.Nodes.InUse[node_idx])
        {
            DrawListActivateNodeBackground(node_idx);
            DrawNode(editor, node_idx);
        }
    }

    // In order to render the links underneath the nodes, we want to first select the bottom draw
    // channel.
    GImNodes->CanvasDrawList->ChannelsSetCurrent(0);

    for (int link_idx = 0; link_idx < editor.Links.Pool.size(); ++link_idx)
    {
        if (editor.Links.InUse[link_idx])
        {
            //don't draw selected link as there control primitives are drawn instead
            if (!editor.SelectedLinkIndices.contains(link_idx))
                DrawLink(editor, link_idx);
        }
    }

    for (int label_idx = 0; label_idx < editor.Labels.Pool.size(); ++label_idx)
    {
        if (editor.Labels.InUse[label_idx])
        {
            DrawLabel(editor, label_idx);
        }
    }

    for (int link_control_idx = 0; link_control_idx < editor.LinkControls.Pool.size(); ++link_control_idx)
    {
        if (editor.LinkControls.InUse[link_control_idx])
        {
            const ImLinkControlData& link_control = editor.LinkControls.Pool[link_control_idx];
            const ImLinkData& link = editor.Links.Pool[link_control.LinkIdx];
            const ImPinData& start_pin = editor.Pins.Pool[link.StartPinIdx];
            const ImPinData& end_pin = editor.Pins.Pool[link.EndPinIdx];

            const Curve curve = GetCurve(
                start_pin.Pos, end_pin.Pos, start_pin.Type, end_pin.Type, link.LinkType, link.Deformations);


            ImVector<int> local_ids = GetAllowedLinkControlLocalId(editor, link_control.LinkIdx);
            ImU32 link_control_color = link.ColorStyle.Base;
            if (editor.SelectedLinkControlIndices.contains(link_control_idx))
            {
                link_control_color = link.ColorStyle.Selected;
            }
            else if (GImNodes->HoveredLinkControlIdx == link_control_idx)
            {
                link_control_color = link.ColorStyle.Hovered;
            }

            if(local_ids.contains(link_control.LocalId))
                DrawControlPrimitive(GetControlPrimitive(link_control.LocalId, curve), link_control_color, GImNodes->Style.LinkThickness);
        }
    }

    // Render the click interaction UI elements (partial links, box selector) on top of everything
    // else.

    DrawListAppendClickInteractionChannel();
    DrawListActivateClickInteractionChannel();

    if (IsMiniMapActive())
    {
        CalcMiniMapLayout();
        MiniMapUpdate();
    }

    // Handle node graph interaction

    if (!IsMiniMapHovered())
    {
        if (GImNodes->LeftMouseClicked && GImNodes->HoveredLinkIdx.HasValue())
        {
            BeginLinkInteraction(editor, GImNodes->HoveredLinkIdx.Value(), GImNodes->HoveredPinIdx);
        }

        else if (GImNodes->LeftMouseClicked && GImNodes->HoveredPinIdx.HasValue())
        {
            BeginLinkCreation(editor, GImNodes->HoveredPinIdx.Value());
        }

        else if (GImNodes->LeftMouseClicked && GImNodes->HoveredNodeIdx.HasValue())
        {
            BeginNodeSelection(editor, GImNodes->HoveredNodeIdx.Value());
        }
        else if (GImNodes->LeftMouseClicked && GImNodes->HoveredLinkControlIdx.HasValue())
        {
            BeginLinkControlSelection(editor, GImNodes->HoveredLinkControlIdx.Value());
        }
        else if (GImNodes->LeftMouseClicked && GImNodes->HoveredLabelIdx.HasValue())
        {
            BeginLabelSelection(editor, GImNodes->HoveredLabelIdx.Value());
        }

        else if (
            GImNodes->LeftMouseClicked || GImNodes->LeftMouseReleased ||
            GImNodes->AltMouseClicked || GImNodes->AltMouseScrollDelta != 0.f)
        {
            BeginCanvasInteraction(editor);
        }

        bool should_auto_pan =
            editor.ClickInteraction.Type == ImNodesClickInteractionType_BoxSelection ||
            editor.ClickInteraction.Type == ImNodesClickInteractionType_LinkCreation ||
            editor.ClickInteraction.Type == ImNodesClickInteractionType_Node;
        if (should_auto_pan && !MouseInCanvas())
        {
            ImVec2 mouse = ImGui::GetMousePos();
            ImVec2 center = GImNodes->CanvasRectScreenSpace.GetCenter();
            ImVec2 direction = (center - mouse);
            direction = direction * ImInvLength(direction, 0.0);

            editor.AutoPanningDelta =
                direction * ImGui::GetIO().DeltaTime * GImNodes->Io.AutoPanningSpeed;
            editor.Panning += editor.AutoPanningDelta;
        }
    }
    ClickInteractionUpdate(editor);

    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_Z)) {
        PopEventVar();
    }
    else if(ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_Y)){
        UnpopEventVar();
    }

    // At this point, draw commands have been issued for all nodes (and pins). Update the node pool
    // to detect unused node slots and remove those indices from the depth stack before sorting the
    // node draw commands by depth.
    ObjectPoolUpdate(editor.Nodes);
    ObjectPoolUpdate(editor.Pins);

    DrawListSortChannelsByDepth(editor.NodeDepthOrder);

    // After the links have been rendered, the link pool can be updated as well.
    ObjectPoolUpdate(editor.Links);

    // Finally, merge the draw channels
    GImNodes->CanvasDrawList->ChannelsMerge();

    // pop style
    ImGui::EndChild();      // end scrolling region
    ImGui::PopStyleColor(); // pop child window background color
    ImGui::PopStyleVar();   // pop window padding
    ImGui::PopStyleVar();   // pop frame padding
    ImGui::EndGroup();
}

void MiniMap(
    const float                                      minimap_size_fraction,
    const ImNodesMiniMapLocation                     location,
    const ImNodesMiniMapNodeHoveringCallback         node_hovering_callback,
    const ImNodesMiniMapNodeHoveringCallbackUserData node_hovering_callback_data)
{
    // Check that editor size fraction is sane; must be in the range (0, 1]
    IM_ASSERT(minimap_size_fraction > 0.f && minimap_size_fraction <= 1.f);

    // Remember to call before EndNodeEditor
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_Editor);

    ImNodesEditorContext& editor = EditorContextGet();

    editor.MiniMapEnabled = true;
    editor.MiniMapSizeFraction = minimap_size_fraction;
    editor.MiniMapLocation = location;

    // Set node hovering callback information
    editor.MiniMapNodeHoveringCallback = node_hovering_callback;
    editor.MiniMapNodeHoveringCallbackUserData = node_hovering_callback_data;

    // Actual drawing/updating of the MiniMap is done in EndNodeEditor so that
    // mini map is draw over everything and all pin/link positions are updated
    // correctly relative to their respective nodes. Hence, we must store some of
    // of the state for the mini map in GImNodes for the actual drawing/updating
}

void BeginNode(const int node_id)
{
    // Remember to call BeginNodeEditor before calling BeginNode
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_Editor);
    GImNodes->CurrentScope = ImNodesScope_Node;

    ImNodesEditorContext& editor = EditorContextGet();

    const int node_idx = ObjectPoolFindOrCreateIndex(editor.Nodes, node_id);
    GImNodes->CurrentNodeIdx = node_idx;

    ImNodeData& node = editor.Nodes.Pool[node_idx];
    node.ColorStyle.Background = GImNodes->Style.Colors[ImNodesCol_NodeBackground];
    node.ColorStyle.BackgroundHovered = GImNodes->Style.Colors[ImNodesCol_NodeBackgroundHovered];
    node.ColorStyle.BackgroundSelected = GImNodes->Style.Colors[ImNodesCol_NodeBackgroundSelected];
    node.ColorStyle.Outline = GImNodes->Style.Colors[ImNodesCol_NodeOutline];
    node.ColorStyle.Titlebar = GImNodes->Style.Colors[ImNodesCol_TitleBar];
    node.ColorStyle.TitlebarHovered = GImNodes->Style.Colors[ImNodesCol_TitleBarHovered];
    node.ColorStyle.TitlebarSelected = GImNodes->Style.Colors[ImNodesCol_TitleBarSelected];
    node.LayoutStyle.CornerRounding = GImNodes->Style.NodeCornerRounding;
    node.LayoutStyle.Padding = GImNodes->Style.NodePadding;
    node.LayoutStyle.BorderThickness = GImNodes->Style.NodeBorderThickness;

    // ImGui::SetCursorPos sets the cursor position, local to the current widget
    // (in this case, the child object started in BeginNodeEditor). Use
    // ImGui::SetCursorScreenPos to set the screen space coordinates directly.
    ImGui::SetCursorPos(GridSpaceToEditorSpace(editor, GetNodeTitleBarOrigin(node)));

    DrawListAddNode(node_idx);
    DrawListActivateCurrentNodeForeground();

    ImGui::PushID(node.Id);
    ImGui::BeginGroup();
}

void EndNode()
{
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_Node);
    GImNodes->CurrentScope = ImNodesScope_Editor;

    ImNodesEditorContext& editor = EditorContextGet();

    // The node's rectangle depends on the ImGui UI group size.
    ImGui::EndGroup();
    ImGui::PopID();

    ImNodeData& node = editor.Nodes.Pool[GImNodes->CurrentNodeIdx];
    node.Rect = GetItemRect();
    node.Rect.Expand(node.LayoutStyle.Padding);

    editor.GridContentBounds.Add(node.Origin);
    editor.GridContentBounds.Add(node.Origin + node.Rect.GetSize());

    if (node.Rect.Contains(GImNodes->MousePos))
    {
        GImNodes->NodeIndicesOverlappingWithMouse.push_back(GImNodes->CurrentNodeIdx);
    }
}

ImVec2 GetNodeDimensions(int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    const int             node_idx = ObjectPoolFind(editor.Nodes, node_id);
    IM_ASSERT(node_idx != -1); // invalid node_id
    const ImNodeData& node = editor.Nodes.Pool[node_idx];
    return node.Rect.GetSize();
}

void BeginNodeTitleBar()
{
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_Node);
    ImGui::BeginGroup();
}

void EndNodeTitleBar()
{
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_Node);
    ImGui::EndGroup();

    ImNodesEditorContext& editor = EditorContextGet();
    ImNodeData&           node = editor.Nodes.Pool[GImNodes->CurrentNodeIdx];
    node.TitleBarContentRect = GetItemRect();

    ImGui::ItemAdd(GetNodeTitleRect(node), ImGui::GetID("title_bar"));

    ImGui::SetCursorPos(GridSpaceToEditorSpace(editor, GetNodeContentOrigin(node)));
}

void BeginInputAttribute(const int id, const ImNodesPinShape shape)
{
    BeginPinAttribute(id, ImNodesAttributeType_Input, shape, GImNodes->CurrentNodeIdx);
}

void EndInputAttribute() { EndPinAttribute(); }

void BeginOutputAttribute(const int id, const ImNodesPinShape shape)
{
    BeginPinAttribute(id, ImNodesAttributeType_Output, shape, GImNodes->CurrentNodeIdx);
}

void EndOutputAttribute() { EndPinAttribute(); }

void BeginStaticAttribute(const int id)
{
    // Make sure to call BeginNode() before calling BeginAttribute()
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_Node);
    GImNodes->CurrentScope = ImNodesScope_Attribute;

    BeginSwappableAttribute();

    GImNodes->CurrentAttributeId = id;

    ImGui::BeginGroup();
    ImGui::PushID(id);
}

void EndStaticAttribute()
{
    // Make sure to call BeginNode() before calling BeginAttribute()
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_Attribute);
    GImNodes->CurrentScope = ImNodesScope_Node;

    ImGui::PopID();
    ImGui::EndGroup();

    if (ImGui::IsItemActive())
    {
        GImNodes->ActiveAttribute = true;
        GImNodes->ActiveAttributeId = GImNodes->CurrentAttributeId;
    }
    EndSwappableAttribute();
}

void PushAttributeFlag(const ImNodesAttributeFlags flag)
{
    GImNodes->CurrentAttributeFlags |= flag;
    GImNodes->AttributeFlagStack.push_back(GImNodes->CurrentAttributeFlags);
}

void PopAttributeFlag()
{
    // PopAttributeFlag called without a matching PushAttributeFlag!
    // The bottom value is always the default value, pushed in Initialize().
    IM_ASSERT(GImNodes->AttributeFlagStack.size() > 1);

    GImNodes->AttributeFlagStack.pop_back();
    GImNodes->CurrentAttributeFlags = GImNodes->AttributeFlagStack.back();
}

void LinkControl(ImNodesEditorContext& editor, int link_idx) {

    ImVector<int> local_Ids = GetAllowedLinkControlLocalId(editor, link_idx);
    for (int i = 0; i < local_Ids.size(); i++) {

        const ImLinkData& link = editor.Links.Pool[link_idx];

        int Id = GetLinkControlId(local_Ids[i], link.Id);
        ImLinkControlData& link_control = ObjectPoolFindOrCreateObject(editor.LinkControls, Id);
        link_control.LinkIdx = link_idx;
        link_control.LocalId = local_Ids[i];
        link_control.Id = Id;
    }
}

void Link(const int id, const int start_attr_id, const int end_attr_id, ImNodesLinkType linkType)
{
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_Editor);

    ImNodesEditorContext& editor = EditorContextGet();
    ImLinkData&           link = ObjectPoolFindOrCreateObject(editor.Links, id);
    link.Id = id;
    link.StartPinIdx = ObjectPoolFindOrCreateIndex(editor.Pins, start_attr_id);
    link.EndPinIdx = ObjectPoolFindOrCreateIndex(editor.Pins, end_attr_id);
    link.ColorStyle.Base = GImNodes->Style.Colors[ImNodesCol_Link];
    link.ColorStyle.Hovered = GImNodes->Style.Colors[ImNodesCol_LinkHovered];
    link.ColorStyle.Selected = GImNodes->Style.Colors[ImNodesCol_LinkSelected];
    link.LinkType = linkType;

    // Check if this link was created by the current link event
    if ((editor.ClickInteraction.Type == ImNodesClickInteractionType_LinkCreation &&
         editor.Pins.Pool[link.EndPinIdx].Flags & ImNodesAttributeFlags_EnableLinkCreationOnSnap &&
         editor.ClickInteraction.LinkCreation.StartPinIdx == link.StartPinIdx &&
         editor.ClickInteraction.LinkCreation.EndPinIdx == link.EndPinIdx) ||
        (editor.ClickInteraction.LinkCreation.StartPinIdx == link.EndPinIdx &&
         editor.ClickInteraction.LinkCreation.EndPinIdx == link.StartPinIdx))
    {
        GImNodes->SnapLinkIdx = ObjectPoolFindOrCreateIndex(editor.Links, id);
    }

    int link_idx = ObjectPoolFindOrCreateIndex(editor.Links, id);
    if(editor.SelectedLinkIndices.contains(link_idx))
        LinkControl(editor, link_idx);
}

void BeginLabel(int parentId, int labelId, ImNodesParentLabelType parentType) {
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_Editor);
    GImNodes->CurrentScope = ImNodesScope_Label;

    ImNodesEditorContext& editor = EditorContextGet();

    const int label_idx = ObjectPoolFindOrCreateIndex(editor.Labels, labelId);

    ImLabelData& label = editor.Labels.Pool[label_idx];
    GImNodes->CurrentLabelIdx = label_idx;

    label.LayoutStyle.Padding = GImNodes->Style.LabelPadding;
    label.LayoutStyle.CornerRounding = GImNodes->Style.LabelCornerRounding;
    label.ColorStyle.BorderHovered = GImNodes->Style.Colors[ImNodesCol_LabelHovered];
    label.ColorStyle.BorderSelected = GImNodes->Style.Colors[ImNodesCol_LabelSelected];
    label.Draggable = GImNodes->Style.LabelDraggable;
    label.parentId = parentId;
    label.parentType = parentType;
    label._Origin = GetLabelOrigin(editor, label) + label.Deformation;

    ImGui::SetCursorPos(label._Origin);

    ImGui::PushID(ImGui::GetID(label.Id));
    ImGui::BeginGroup();
}

void BeginPinLabel(int pinId, int labelId) {
    BeginLabel(pinId, labelId, ImNodesParentLabelType_Pin);
}

void BeginLinkLabel(int linkId, int labelId) {
    BeginLabel(linkId, labelId, ImNodesParentLabelType_Link);
}

void EndLabel() {
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_Label);
    GImNodes->CurrentScope = ImNodesScope_Editor;

    ImNodesEditorContext& editor = EditorContextGet();
    
    ImLabelData& label = editor.Labels.Pool[GImNodes->CurrentLabelIdx];
    
    label.Rect = GetItemRect();

    ImGui::EndGroup();
    ImGui::PopID();

    label.Rect.Expand(label.LayoutStyle.Padding);

    editor.GridContentBounds.Add(label.Rect.Min);
    editor.GridContentBounds.Add(label.Rect.Max);
}

void EndPinLabel() {
    EndLabel();
}

void EndLinkLabel() {
    EndLabel();
}

void PushColorStyle(const ImNodesCol item, unsigned int color)
{
    GImNodes->ColorModifierStack.push_back(ImNodesColElement(GImNodes->Style.Colors[item], item));
    GImNodes->Style.Colors[item] = color;
}

void PopColorStyle()
{
    IM_ASSERT(GImNodes->ColorModifierStack.size() > 0);
    const ImNodesColElement elem = GImNodes->ColorModifierStack.back();
    GImNodes->Style.Colors[elem.Item] = elem.Color;
    GImNodes->ColorModifierStack.pop_back();
}

struct ImNodesStyleVarInfo
{
    ImGuiDataType Type;
    ImU32         Count;
    ImU32         Offset;
    void* GetVarPtr(ImNodesStyle* style) const { return (void*)((unsigned char*)style + Offset); }
};

static const ImNodesStyleVarInfo GStyleVarInfo[] = {
    // ImNodesStyleVar_GridSpacing
    {ImGuiDataType_Float, 1, (ImU32)offsetof(ImNodesStyle, GridSpacing)},
    // ImNodesStyleVar_NodeCornerRounding
    {ImGuiDataType_Float, 1, (ImU32)offsetof(ImNodesStyle, NodeCornerRounding)},
    // ImNodesStyleVar_NodePadding
    {ImGuiDataType_Float, 2, (ImU32)offsetof(ImNodesStyle, NodePadding)},
    // ImNodesStyleVar_NodeBorderThickness
    {ImGuiDataType_Float, 1, (ImU32)offsetof(ImNodesStyle, NodeBorderThickness)},
    // ImNodesStyleVar_LinkThickness
    {ImGuiDataType_Float, 1, (ImU32)offsetof(ImNodesStyle, LinkThickness)},
    // ImNodesStyleVar_LinkLineSegmentsPerLength
    {ImGuiDataType_Float, 1, (ImU32)offsetof(ImNodesStyle, LinkLineSegmentsPerLength)},
    // ImNodesStyleVar_LinkHoverDistance
    {ImGuiDataType_Float, 1, (ImU32)offsetof(ImNodesStyle, LinkHoverDistance)},
    // ImNodesStyleVar_LinkSlopedMinSlope
    {ImGuiDataType_Float, 1, (ImU32)offsetof(ImNodesStyle, LinkSlopedMinSlope)},
    // ImNodesStyleVar_LinkSlopedMinOffset
    {ImGuiDataType_Float, 1, (ImU32)offsetof(ImNodesStyle, LinkSlopedMinOffset)},
    // ImNodesStyleVar_LinkCreationType
    {ImGuiDataType_U32, 1, (ImU32)offsetof(ImNodesStyle, LinkCreationType)},
    // ImNodesStyleVar_LabelPadding
    {ImGuiDataType_Float, 2, (ImU32)offsetof(ImNodesStyle, LabelPadding)},
    // ImNodesStyleVar_LabelCornerRounding
    {ImGuiDataType_Float, 1, (ImU32)offsetof(ImNodesStyle, LabelCornerRounding)},
    // ImNodesStyleVar_LabelDraggable
    {ImGuiDataType_Bool, 1, (ImU32)offsetof(ImNodesStyle, LabelDraggable)},
    // ImNodesStyleVar_PinCircleRadius
    {ImGuiDataType_Float, 1, (ImU32)offsetof(ImNodesStyle, PinCircleRadius)},
    // ImNodesStyleVar_PinQuadSideLength
    {ImGuiDataType_Float, 1, (ImU32)offsetof(ImNodesStyle, PinQuadSideLength)},
    // ImNodesStyleVar_PinTriangleSideLength
    {ImGuiDataType_Float, 1, (ImU32)offsetof(ImNodesStyle, PinTriangleSideLength)},
    // ImNodesStyleVar_PinLineThickness
    {ImGuiDataType_Float, 1, (ImU32)offsetof(ImNodesStyle, PinLineThickness)},
    // ImNodesStyleVar_PinHoverRadius
    {ImGuiDataType_Float, 1, (ImU32)offsetof(ImNodesStyle, PinHoverRadius)},
    // ImNodesStyleVar_PinOffset
    {ImGuiDataType_Float, 1, (ImU32)offsetof(ImNodesStyle, PinOffset)},
    // ImNodesStyleVar_MiniMapPadding
    {ImGuiDataType_Float, 2, (ImU32)offsetof(ImNodesStyle, MiniMapPadding)},
    // ImNodesStyleVar_MiniMapOffset
    {ImGuiDataType_Float, 2, (ImU32)offsetof(ImNodesStyle, MiniMapOffset)},
};

static const ImNodesStyleVarInfo* GetStyleVarInfo(ImNodesStyleVar idx)
{
    IM_ASSERT(idx >= 0 && idx < ImNodesStyleVar_COUNT);
    IM_ASSERT(IM_ARRAYSIZE(GStyleVarInfo) == ImNodesStyleVar_COUNT);
    return &GStyleVarInfo[idx];
}

void PushStyleVar(const ImNodesStyleVar item, const int value)
{
    const ImNodesStyleVarInfo* var_info = GetStyleVarInfo(item);
    if (var_info->Type == ImGuiDataType_U32 && var_info->Count == 1)
    {
        int& style_var = *(int*)var_info->GetVarPtr(&GImNodes->Style);
        GImNodes->StyleModifierStack.push_back(ImNodesStyleVarElement(item, style_var));
        style_var = value;
        return;
    }
    IM_ASSERT(0 && "Called PushStyleVar() int variant but variable is not a int!");
}

void PushStyleVar(const ImNodesStyleVar item, const float value)
{
    const ImNodesStyleVarInfo* var_info = GetStyleVarInfo(item);
    if (var_info->Type == ImGuiDataType_Float && var_info->Count == 1)
    {
        float& style_var = *(float*)var_info->GetVarPtr(&GImNodes->Style);
        GImNodes->StyleModifierStack.push_back(ImNodesStyleVarElement(item, style_var));
        style_var = value;
        return;
    }
    IM_ASSERT(0 && "Called PushStyleVar() float variant but variable is not a float!");
}

void PushStyleVar(const ImNodesStyleVar item, const ImVec2& value)
{
    const ImNodesStyleVarInfo* var_info = GetStyleVarInfo(item);
    if (var_info->Type == ImGuiDataType_Float && var_info->Count == 2)
    {
        ImVec2& style_var = *(ImVec2*)var_info->GetVarPtr(&GImNodes->Style);
        GImNodes->StyleModifierStack.push_back(ImNodesStyleVarElement(item, style_var));
        style_var = value;
        return;
    }
    IM_ASSERT(0 && "Called PushStyleVar() ImVec2 variant but variable is not a ImVec2!");
}

void PushStyleVar(const ImNodesStyleVar item, const bool& value)
{
    const ImNodesStyleVarInfo* var_info = GetStyleVarInfo(item);
    if (var_info->Type == ImGuiDataType_Bool && var_info->Count == 1)
    {
        int& style_var = *(int*)var_info->GetVarPtr(&GImNodes->Style);
        GImNodes->StyleModifierStack.push_back(ImNodesStyleVarElement(item, style_var));
        style_var = value ? 1 : 0;
        return;
    }
    IM_ASSERT(0 && "Called PushStyleVar() bool variant but variable is not a bool!");
}

void PopStyleVar(int count)
{
    while (count > 0)
    {
        IM_ASSERT(GImNodes->StyleModifierStack.size() > 0);
        const ImNodesStyleVarElement style_backup = GImNodes->StyleModifierStack.back();
        GImNodes->StyleModifierStack.pop_back();
        const ImNodesStyleVarInfo* var_info = GetStyleVarInfo(style_backup.Item);
        void*                      style_var = var_info->GetVarPtr(&GImNodes->Style);
        if (var_info->Type == ImGuiDataType_U32 && var_info->Count == 1)
        {
            ((int*)style_var)[0] = style_backup.IntValue;
        }
        else if (var_info->Type == ImGuiDataType_Float && var_info->Count == 1)
        {
            ((float*)style_var)[0] = style_backup.FloatValue[0];
        }
        else if (var_info->Type == ImGuiDataType_Float && var_info->Count == 2)
        {
            ((float*)style_var)[0] = style_backup.FloatValue[0];
            ((float*)style_var)[1] = style_backup.FloatValue[1];
        }
        else if (var_info->Type == ImGuiDataType_Bool && var_info->Count == 1)
        {
            ((bool*)style_var)[0] = style_backup.IntValue == 1;
        }
        count--;
    }
}

void PushEvent(int idx) {
    PushEventVar(ImNodesEventVarElement(ImNodesEventVar_UserEvent, idx));
}

bool GetPopedEvent(int* idx) {
    if (GImNodes->PopedEvent.HasValue()) {
        *idx = GImNodes->PopedEvent.Value();
        return true;
    }
    return false;
}

bool GetUnpopedEvent(int* idx) {
    if (GImNodes->UnpopedEvent.HasValue()) {
        *idx = GImNodes->UnpopedEvent.Value();
        return true;
    }
    return false;
}

void SetNodeScreenSpacePos(const int node_id, const ImVec2& screen_space_pos)
{
    ImNodesEditorContext& editor = EditorContextGet();
    ImNodeData&           node = ObjectPoolFindOrCreateObject(editor.Nodes, node_id);
    node.Origin = ScreenSpaceToGridSpace(editor, screen_space_pos);
}

void SetNodeEditorSpacePos(const int node_id, const ImVec2& editor_space_pos)
{
    ImNodesEditorContext& editor = EditorContextGet();
    ImNodeData&           node = ObjectPoolFindOrCreateObject(editor.Nodes, node_id);
    node.Origin = EditorSpaceToGridSpace(editor, editor_space_pos);
}

void SetNodeGridSpacePos(const int node_id, const ImVec2& grid_pos)
{
    ImNodesEditorContext& editor = EditorContextGet();
    ImNodeData&           node = ObjectPoolFindOrCreateObject(editor.Nodes, node_id);
    node.Origin = grid_pos;
}

void SetNodeDraggable(const int node_id, const bool draggable)
{
    ImNodesEditorContext& editor = EditorContextGet();
    ImNodeData&           node = ObjectPoolFindOrCreateObject(editor.Nodes, node_id);
    node.Draggable = draggable;
}

ImVec2 GetNodeScreenSpacePos(const int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    const int             node_idx = ObjectPoolFind(editor.Nodes, node_id);
    IM_ASSERT(node_idx != -1);
    ImNodeData& node = editor.Nodes.Pool[node_idx];
    return GridSpaceToScreenSpace(editor, node.Origin);
}

ImVec2 GetNodeEditorSpacePos(const int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    const int             node_idx = ObjectPoolFind(editor.Nodes, node_id);
    IM_ASSERT(node_idx != -1);
    ImNodeData& node = editor.Nodes.Pool[node_idx];
    return GridSpaceToEditorSpace(editor, node.Origin);
}

ImVec2 GetNodeGridSpacePos(const int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    const int             node_idx = ObjectPoolFind(editor.Nodes, node_id);
    IM_ASSERT(node_idx != -1);
    ImNodeData& node = editor.Nodes.Pool[node_idx];
    return node.Origin;
}

void SnapNodeToGrid(int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    ImNodeData&           node = ObjectPoolFindOrCreateObject(editor.Nodes, node_id);
    node.Origin = SnapOriginToGrid(node.Origin);
}

bool IsEditorHovered() { return MouseInCanvas(); }

bool IsNodeHovered(int* const node_id)
{
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_None);
    IM_ASSERT(node_id != NULL);

    const bool is_hovered = GImNodes->HoveredNodeIdx.HasValue();
    if (is_hovered)
    {
        const ImNodesEditorContext& editor = EditorContextGet();
        *node_id = editor.Nodes.Pool[GImNodes->HoveredNodeIdx.Value()].Id;
    }
    return is_hovered;
}

bool IsLinkHovered(int* const link_id)
{
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_None);
    IM_ASSERT(link_id != NULL);

    const bool is_hovered = GImNodes->HoveredLinkIdx.HasValue();
    if (is_hovered)
    {
        const ImNodesEditorContext& editor = EditorContextGet();
        *link_id = editor.Links.Pool[GImNodes->HoveredLinkIdx.Value()].Id;
    }
    return is_hovered;
}

bool IsPinHovered(int* const attr)
{
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_None);
    IM_ASSERT(attr != NULL);

    const bool is_hovered = GImNodes->HoveredPinIdx.HasValue();
    if (is_hovered)
    {
        const ImNodesEditorContext& editor = EditorContextGet();
        *attr = editor.Pins.Pool[GImNodes->HoveredPinIdx.Value()].Id;
    }
    return is_hovered;
}

bool IsLabelHovered(int* const label_id)
{

    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_None);
    IM_ASSERT(label_id != NULL);
    const bool is_hovered = GImNodes->HoveredLabelIdx.HasValue();
    if (is_hovered)
    {
        const ImNodesEditorContext& editor = EditorContextGet();
        *label_id = editor.Labels.Pool[GImNodes->HoveredLabelIdx.Value()].Id;
    }
    return is_hovered;
}

int NumSelectedNodes()
{
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_None);
    const ImNodesEditorContext& editor = EditorContextGet();
    return editor.SelectedNodeIndices.size();
}

int NumSelectedLinks()
{
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_None);
    const ImNodesEditorContext& editor = EditorContextGet();
    return editor.SelectedLinkIndices.size();
}

int NumSelectedLabels()
{
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_None);
    const ImNodesEditorContext& editor = EditorContextGet();
    return editor.SelectedLabelIndices.size();
}

void GetSelectedNodes(int* node_ids)
{
    IM_ASSERT(node_ids != NULL);

    const ImNodesEditorContext& editor = EditorContextGet();
    for (int i = 0; i < editor.SelectedNodeIndices.size(); ++i)
    {
        const int node_idx = editor.SelectedNodeIndices[i];
        node_ids[i] = editor.Nodes.Pool[node_idx].Id;
    }
}

void GetSelectedLinks(int* link_ids)
{
    IM_ASSERT(link_ids != NULL);

    const ImNodesEditorContext& editor = EditorContextGet();
    for (int i = 0; i < editor.SelectedLinkIndices.size(); ++i)
    {
        const int link_idx = editor.SelectedLinkIndices[i];
        link_ids[i] = editor.Links.Pool[link_idx].Id;
    }
}

void GetSelectedLabels(int* label_ids)
{
    IM_ASSERT(label_ids != NULL);

    const ImNodesEditorContext& editor = EditorContextGet();
    for (int i = 0; i < editor.SelectedLabelIndices.size(); ++i)
    {
        const int label_idx = editor.SelectedLabelIndices[i];
        label_ids[i] = editor.Labels.Pool[label_idx].Id;
    }
}

void ClearNodeSelection()
{
    ImNodesEditorContext& editor = EditorContextGet();
    editor.SelectedNodeIndices.clear();
}

void ClearNodeSelection(int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    ClearObjectSelection(editor.Nodes, editor.SelectedNodeIndices, node_id);
}

void ClearLinkSelection()
{
    ImNodesEditorContext& editor = EditorContextGet();
    editor.SelectedLinkIndices.clear();
}

void ClearLabelSelection()
{
    ImNodesEditorContext& editor = EditorContextGet();
    editor.SelectedLabelIndices.clear();
}

void ClearLinkSelection(int link_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    ClearObjectSelection(editor.Links, editor.SelectedLinkIndices, link_id);
}

void ClearLabelSelection(int label_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    ClearObjectSelection(editor.Labels, editor.SelectedLabelIndices, label_id);
}

void SelectNode(int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    SelectObject(editor.Nodes, editor.SelectedNodeIndices, node_id);
}

void SelectLink(int link_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    SelectObject(editor.Links, editor.SelectedLinkIndices, link_id);
}

void SelectLabel(int label_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    SelectObject(editor.Labels, editor.SelectedLabelIndices, label_id);
}

bool IsNodeSelected(int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    return IsObjectSelected(editor.Nodes, editor.SelectedNodeIndices, node_id);
}

bool IsLinkSelected(int link_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    return IsObjectSelected(editor.Links, editor.SelectedLinkIndices, link_id);
}

bool IsLabelSelected(int label_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    return IsObjectSelected(editor.Labels, editor.SelectedLabelIndices, label_id);
}

bool IsAttributeActive()
{
    IM_ASSERT((GImNodes->CurrentScope & ImNodesScope_Node) != 0);

    if (!GImNodes->ActiveAttribute)
    {
        return false;
    }

    return GImNodes->ActiveAttributeId == GImNodes->CurrentAttributeId;
}

bool IsAnyAttributeActive(int* const attribute_id)
{
    IM_ASSERT((GImNodes->CurrentScope & (ImNodesScope_Node | ImNodesScope_Attribute)) == 0);

    if (!GImNodes->ActiveAttribute)
    {
        return false;
    }

    if (attribute_id != NULL)
    {
        *attribute_id = GImNodes->ActiveAttributeId;
    }

    return true;
}

bool IsLinkStarted(int* const started_at_id)
{
    // Call this function after EndNodeEditor()!
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_None);
    IM_ASSERT(started_at_id != NULL);

    const bool is_started = (GImNodes->ImNodesUIState & ImNodesUIState_LinkStarted) != 0;
    if (is_started)
    {
        const ImNodesEditorContext& editor = EditorContextGet();
        const int                   pin_idx = editor.ClickInteraction.LinkCreation.StartPinIdx;
        *started_at_id = editor.Pins.Pool[pin_idx].Id;
    }

    return is_started;
}

bool IsLinkDropped(int* const started_at_id, const bool including_detached_links)
{
    // Call this function after EndNodeEditor()!
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_None);

    const ImNodesEditorContext& editor = EditorContextGet();

    const bool link_dropped =
        (GImNodes->ImNodesUIState & ImNodesUIState_LinkDropped) != 0 &&
        (including_detached_links ||
         editor.ClickInteraction.LinkCreation.Type != ImNodesLinkCreationType_FromDetach);

    if (link_dropped && started_at_id)
    {
        const int pin_idx = editor.ClickInteraction.LinkCreation.StartPinIdx;
        *started_at_id = editor.Pins.Pool[pin_idx].Id;
    }

    return link_dropped;
}

bool IsLinkCreated(
    int* const             started_at_pin_id,
    int* const             ended_at_pin_id,
    bool* const            created_from_snap,
    ImNodesLinkType* const link_type)
{
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_None);
    IM_ASSERT(started_at_pin_id != NULL);
    IM_ASSERT(ended_at_pin_id != NULL);

    const bool is_created = (GImNodes->ImNodesUIState & ImNodesUIState_LinkCreated) != 0;

    if (is_created)
    {
        const ImNodesEditorContext& editor = EditorContextGet();
        const int                   start_idx = editor.ClickInteraction.LinkCreation.StartPinIdx;
        const int        end_idx = editor.ClickInteraction.LinkCreation.EndPinIdx.Value();
        const ImPinData& start_pin = editor.Pins.Pool[start_idx];
        const ImPinData& end_pin = editor.Pins.Pool[end_idx];

        if (start_pin.Type == ImNodesAttributeType_Output)
        {
            *started_at_pin_id = start_pin.Id;
            *ended_at_pin_id = end_pin.Id;
        }
        else
        {
            *started_at_pin_id = end_pin.Id;
            *ended_at_pin_id = start_pin.Id;
        }

        if (created_from_snap)
        {
            *created_from_snap =
                editor.ClickInteraction.Type == ImNodesClickInteractionType_LinkCreation;
        }
        if (link_type)
        {
            *link_type = GImNodes->Style.LinkCreationType;
        }
    }

    return is_created;
}

bool IsLinkCreated(
    int*                   started_at_node_id,
    int*                   started_at_pin_id,
    int*                   ended_at_node_id,
    int*                   ended_at_pin_id,
    bool*                  created_from_snap,
    ImNodesLinkType* const link_type)
{
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_None);
    IM_ASSERT(started_at_node_id != NULL);
    IM_ASSERT(started_at_pin_id != NULL);
    IM_ASSERT(ended_at_node_id != NULL);
    IM_ASSERT(ended_at_pin_id != NULL);

    const bool is_created = (GImNodes->ImNodesUIState & ImNodesUIState_LinkCreated) != 0;

    if (is_created)
    {
        const ImNodesEditorContext& editor = EditorContextGet();
        const int                   start_idx = editor.ClickInteraction.LinkCreation.StartPinIdx;
        const int         end_idx = editor.ClickInteraction.LinkCreation.EndPinIdx.Value();
        const ImPinData&  start_pin = editor.Pins.Pool[start_idx];
        const ImNodeData& start_node = editor.Nodes.Pool[start_pin.ParentNodeIdx];
        const ImPinData&  end_pin = editor.Pins.Pool[end_idx];
        const ImNodeData& end_node = editor.Nodes.Pool[end_pin.ParentNodeIdx];

        if (start_pin.Type == ImNodesAttributeType_Output)
        {
            *started_at_pin_id = start_pin.Id;
            *started_at_node_id = start_node.Id;
            *ended_at_pin_id = end_pin.Id;
            *ended_at_node_id = end_node.Id;
        }
        else
        {
            *started_at_pin_id = end_pin.Id;
            *started_at_node_id = end_node.Id;
            *ended_at_pin_id = start_pin.Id;
            *ended_at_node_id = start_node.Id;
        }

        if (created_from_snap)
        {
            *created_from_snap =
                editor.ClickInteraction.Type == ImNodesClickInteractionType_LinkCreation;
        }
        if (link_type)
        {
            *link_type = GImNodes->Style.LinkCreationType;
        }
    }

    return is_created;
}

bool IsLinkDestroyed(int* const link_id)
{
    IM_ASSERT(GImNodes->CurrentScope == ImNodesScope_None);

    const bool link_destroyed = GImNodes->DeletedLinkIdx.HasValue();
    if (link_destroyed)
    {
        const ImNodesEditorContext& editor = EditorContextGet();
        const int                   link_idx = GImNodes->DeletedLinkIdx.Value();
        *link_id = editor.Links.Pool[link_idx].Id;
    }

    return link_destroyed;
}

bool IsAttributeSwapped(int* src_attr, int* dest_attr, ImNodesAttributeSwapFlags flags) {
    if (GImNodes->ActiveSwappableAttribute && GImNodes->HoveredSwappableAttribute) {
        if (GImNodes->ActiveSwappableAttributeId != GImNodes->HoveredSwappableAttributeId) {
            ImNodesEditorContext& editor = EditorContextGet();
            const int active_pin_idx = ObjectPoolFindOrCreateIndex(editor.Pins, GImNodes->ActiveSwappableAttributeId);
            const int hovered_pin_idx = ObjectPoolFindOrCreateIndex(editor.Pins, GImNodes->HoveredSwappableAttributeId);

            ImPinData& active_pin = editor.Pins.Pool[active_pin_idx];
            ImPinData& hovered_pin = editor.Pins.Pool[hovered_pin_idx];

            if (flags & ImNodesAttributeSwapFlags_SameNode && active_pin.ParentNodeIdx != hovered_pin.ParentNodeIdx) 
                return false;

            if (flags & ImNodesAttributeSwapFlags_SameType && active_pin.Type != hovered_pin.Type) 
                return false;

            if(src_attr != NULL)
                *src_attr = GImNodes->ActiveSwappableAttributeId;
            if (dest_attr != NULL)
                *dest_attr = GImNodes->HoveredSwappableAttributeId;
            return true;
        }
    }
    return false;
}

namespace
{
void NodeLineHandler(ImNodesEditorContext& editor, const char* const line)
{
    int id;
    int x, y;
    if (sscanf(line, "[node.%i", &id) == 1)
    {
        const int node_idx = ObjectPoolFindOrCreateIndex(editor.Nodes, id);
        GImNodes->CurrentNodeIdx = node_idx;
        ImNodeData& node = editor.Nodes.Pool[node_idx];
        node.Id = id;
    }
    else if (sscanf(line, "origin=%i,%i", &x, &y) == 2)
    {
        ImNodeData& node = editor.Nodes.Pool[GImNodes->CurrentNodeIdx];
        node.Origin = SnapOriginToGrid(ImVec2((float)x, (float)y));
    }
}

void EditorLineHandler(ImNodesEditorContext& editor, const char* const line)
{
    (void)sscanf(line, "panning=%f,%f", &editor.Panning.x, &editor.Panning.y);
}
} // namespace

const char* SaveCurrentEditorStateToIniString(size_t* const data_size)
{
    return SaveEditorStateToIniString(&EditorContextGet(), data_size);
}

const char* SaveEditorStateToIniString(
    const ImNodesEditorContext* const editor_ptr,
    size_t* const                     data_size)
{
    IM_ASSERT(editor_ptr != NULL);
    const ImNodesEditorContext& editor = *editor_ptr;

    GImNodes->TextBuffer.clear();
    // TODO: check to make sure that the estimate is the upper bound of element
    GImNodes->TextBuffer.reserve(64 * editor.Nodes.Pool.size());

    GImNodes->TextBuffer.appendf(
        "[editor]\npanning=%i,%i\n", (int)editor.Panning.x, (int)editor.Panning.y);

    for (int i = 0; i < editor.Nodes.Pool.size(); i++)
    {
        if (editor.Nodes.InUse[i])
        {
            const ImNodeData& node = editor.Nodes.Pool[i];
            GImNodes->TextBuffer.appendf("\n[node.%d]\n", node.Id);
            GImNodes->TextBuffer.appendf("origin=%i,%i\n", (int)node.Origin.x, (int)node.Origin.y);
        }
    }

    if (data_size != NULL)
    {
        *data_size = GImNodes->TextBuffer.size();
    }

    return GImNodes->TextBuffer.c_str();
}

void LoadCurrentEditorStateFromIniString(const char* const data, const size_t data_size)
{
    LoadEditorStateFromIniString(&EditorContextGet(), data, data_size);
}

void LoadEditorStateFromIniString(
    ImNodesEditorContext* const editor_ptr,
    const char* const           data,
    const size_t                data_size)
{
    if (data_size == 0u)
    {
        return;
    }

    ImNodesEditorContext& editor = editor_ptr == NULL ? EditorContextGet() : *editor_ptr;

    char*       buf = (char*)ImGui::MemAlloc(data_size + 1);
    const char* buf_end = buf + data_size;
    memcpy(buf, data, data_size);
    buf[data_size] = 0;

    void (*line_handler)(ImNodesEditorContext&, const char*);
    line_handler = NULL;
    char* line_end = NULL;
    for (char* line = buf; line < buf_end; line = line_end + 1)
    {
        while (*line == '\n' || *line == '\r')
        {
            line++;
        }
        line_end = line;
        while (line_end < buf_end && *line_end != '\n' && *line_end != '\r')
        {
            line_end++;
        }
        line_end[0] = 0;

        if (*line == ';' || *line == '\0')
        {
            continue;
        }

        if (line[0] == '[' && line_end[-1] == ']')
        {
            line_end[-1] = 0;
            if (strncmp(line + 1, "node", 4) == 0)
            {
                line_handler = NodeLineHandler;
            }
            else if (strcmp(line + 1, "editor") == 0)
            {
                line_handler = EditorLineHandler;
            }
        }

        if (line_handler != NULL)
        {
            line_handler(editor, line);
        }
    }
    ImGui::MemFree(buf);
}

void SaveCurrentEditorStateToIniFile(const char* const file_name)
{
    SaveEditorStateToIniFile(&EditorContextGet(), file_name);
}

void SaveEditorStateToIniFile(const ImNodesEditorContext* const editor, const char* const file_name)
{
    size_t      data_size = 0u;
    const char* data = SaveEditorStateToIniString(editor, &data_size);
    FILE*       file = ImFileOpen(file_name, "wt");
    if (!file)
    {
        return;
    }

    fwrite(data, sizeof(char), data_size, file);
    fclose(file);
}

void LoadCurrentEditorStateFromIniFile(const char* const file_name)
{
    LoadEditorStateFromIniFile(&EditorContextGet(), file_name);
}

void LoadEditorStateFromIniFile(ImNodesEditorContext* const editor, const char* const file_name)
{
    size_t data_size = 0u;
    char*  file_data = (char*)ImFileLoadToMemory(file_name, "rb", &data_size);

    if (!file_data)
    {
        return;
    }

    LoadEditorStateFromIniString(editor, file_data, data_size);
    ImGui::MemFree(file_data);
}
} // namespace IMNODES_NAMESPACE
