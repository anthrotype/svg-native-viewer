/*
Copyright 2019 Adobe. All rights reserved.
This file is licensed to you under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License. You may obtain a copy
of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under
the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
OF ANY KIND, either express or implied. See the License for the specific language
governing permissions and limitations under the License.
*/

#include "JsonSVGRenderer.h"

#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static json DEFAULT_FILL_COLOR = json({0.0, 0.0, 0.0, 1.0});  // black

namespace SVGNative
{
JsonSVGPath::JsonSVGPath() { }

void JsonSVGPath::Rect(float x, float y, float width, float height)
{
    j["Rect"] = {
        {"x", x},
        {"y", y},
        {"width", width},
        {"height", height}
    };
}

void JsonSVGPath::RoundedRect(float x, float y, float width, float height, float cornerRadius)
{
    j["RoundedRect"] = {
        {"x", x},
        {"y", y},
        {"width", width},
        {"height", height},
        {"cornerRadius", cornerRadius}
    };
}

void JsonSVGPath::Ellipse(float cx, float cy, float rx, float ry)
{
    j["Ellipse"] = {
        {"cx", cx},
        {"cy", cy},
        {"rx", rx},
        {"ry", ry}
    };
}

void JsonSVGPath::MoveTo(float x, float y) {
    json& d = j["d"];
    d.push_back("M");
    d.push_back(x);
    d.push_back(y);
}

void JsonSVGPath::LineTo(float x, float y) {
    json& d = j["d"];
    d.push_back("L");
    d.push_back(x);
    d.push_back(y);
}

void JsonSVGPath::CurveTo(float x1, float y1, float x2, float y2, float x3, float y3)
{
    json& d = j["d"];
    d.push_back("C");
    d.push_back(x1);
    d.push_back(y1);
    d.push_back(x2);
    d.push_back(y2);
    d.push_back(x3);
    d.push_back(y3);
}

void JsonSVGPath::CurveToV(float x2, float y2, float x3, float y3) {
    json& d = j["d"];
    d.push_back("T");
    d.push_back(x2);
    d.push_back(y2);
    d.push_back(x3);
    d.push_back(y3);
}

void JsonSVGPath::ClosePath() {
    j["d"].push_back("Z");
}

float deg2rad(float angle);
float deg2rad(float angle) { return static_cast<float>(M_PI / 180.0 * angle); }

JsonSVGTransform::JsonSVGTransform(float a, float b, float c, float d, float tx, float ty) { Set(a, b, c, d, tx, ty); }

void JsonSVGTransform::Set(float a, float b, float c, float d, float tx, float ty) { mTransform = {a, b, c, d, tx, ty}; }

void JsonSVGTransform::Rotate(float r)
{
    r = deg2rad(r);
    float cosAngle = cos(r);
    float sinAngle = sin(r);

    auto rot = AffineTransform{cosAngle, sinAngle, -sinAngle, cosAngle, 0, 0};
    Multiply(rot);
}

void JsonSVGTransform::Translate(float tx, float ty)
{
    mTransform.e += tx * mTransform.a + ty * mTransform.c;
    mTransform.f += tx * mTransform.b + ty * mTransform.d;
}

void JsonSVGTransform::Scale(float sx, float sy)
{
    mTransform.a *= sx;
    mTransform.b *= sx;
    mTransform.c *= sy;
    mTransform.d *= sy;
}

void JsonSVGTransform::Concat(const Transform& other) { Multiply(static_cast<const JsonSVGTransform&>(other).mTransform); }

json JsonSVGTransform::Json() const
{
    return {
        mTransform.a,
        mTransform.b,
        mTransform.c,
        mTransform.d,
        mTransform.e,
        mTransform.f
    };
}

void JsonSVGTransform::Multiply(const AffineTransform& o)
{
    AffineTransform newT;
    newT.a = o.a * mTransform.a + o.b * mTransform.c;
    newT.b = o.a * mTransform.b + o.b * mTransform.d;
    newT.c = o.c * mTransform.a + o.d * mTransform.c;
    newT.d = o.c * mTransform.b + o.d * mTransform.d;
    newT.e = o.e * mTransform.a + o.f * mTransform.c + mTransform.e;
    newT.f = o.e * mTransform.b + o.f * mTransform.d + mTransform.f;
    mTransform = newT;
}

json JsonSVGImageData::Json() const
{
    return {
        {"base64", mBase64},
        {"encoding", mEncoding}
    };
}

JsonSVGRenderer::JsonSVGRenderer()
{
    mRoot["elements"] = json::array();
    mCurrentGroup = &mRoot;
}

void JsonSVGRenderer::Save(const GraphicStyle& graphicStyle)
{
    json* g = mCurrentGroup;

    if (graphicStyle.opacity != 1.0 || graphicStyle.transform || graphicStyle.clippingPath)
    {
        (*g)["elements"].push_back(json::object());
        mCurrentGroup = &((*g)["elements"].back());
        SaveGraphic(graphicStyle, *mCurrentGroup);
    }

    mGroupStack.push(g);
}

void JsonSVGRenderer::Restore()
{
    if (!mGroupStack.empty())
    {
        mCurrentGroup = mGroupStack.top();
        mGroupStack.pop();
    }
}

void JsonSVGRenderer::DrawPath(
    const Path& path, const GraphicStyle& graphicStyle, const FillStyle& fillStyle, const StrokeStyle& strokeStyle)
{
    json p = static_cast<const JsonSVGPath&>(path).Json();
    SaveGraphic(graphicStyle, p);
    SaveFill(fillStyle, p);
    SaveStroke(strokeStyle, p);
    if (!p.is_null())
        (*mCurrentGroup)["elements"].push_back(p);
}

void JsonSVGRenderer::DrawImage(const ImageData& image, const GraphicStyle& graphicStyle, const Rect& clipArea, const Rect& fillArea)
{
    json i;
    i["clip"] = {clipArea.x, clipArea.y, clipArea.width, clipArea.height};
    i["fill"] = {fillArea.x, fillArea.y, fillArea.width, fillArea.height};
    SaveGraphic(graphicStyle, i);
    json data = static_cast<const JsonSVGImageData&>(image).Json();
    i["base64"] = data["base64"];
    i["encoding"] = data["encoding"];
    (*mCurrentGroup)["elements"].push_back(i);
}

json JsonSVGRenderer::Json() const {
    return mRoot["elements"].empty() ? json({}) : mRoot["elements"].at(0);
}

void JsonSVGRenderer::SaveFill(const FillStyle& fillStyle, json& o)
{
    if (!fillStyle.hasFill)
        return;
    if (fillStyle.fillRule == WindingRule::kEvenOdd)
        o["winding"] = "evenodd";
    if (fillStyle.fillOpacity != 1.0)
        o["opacity"] = fillStyle.fillOpacity;
    SavePaint(fillStyle.paint, o);
}

void JsonSVGRenderer::SaveStroke(const StrokeStyle& strokeStyle, json& o)
{
    if (!strokeStyle.hasStroke)
        return;

    json stroke;
    stroke["width"] = strokeStyle.lineWidth;
    if (strokeStyle.strokeOpacity != 1.0)
        stroke["opacity"] = strokeStyle.strokeOpacity;
    if (strokeStyle.lineCap == LineCap::kButt)
        stroke["cap"] = "butt";
    else if (strokeStyle.lineCap == LineCap::kRound)
        stroke["cap"] = "round";
    else if (strokeStyle.lineCap == LineCap::kSquare)
        stroke["cap"] = "square";
    if (strokeStyle.lineJoin == LineJoin::kMiter)
        stroke["join"] = "miter";
    else if (strokeStyle.lineJoin == LineJoin::kRound)
        stroke["join"] = "round";
    else if (strokeStyle.lineJoin == LineJoin::kBevel)
        stroke["join"] = "bevel";
    stroke["miter"] = strokeStyle.miterLimit;
    if (!strokeStyle.dashArray.empty())
        stroke["dash"] = strokeStyle.dashArray;
    stroke["dashOffset"] = strokeStyle.dashOffset;
    SavePaint(strokeStyle.paint, stroke);

    o["stroke"] = stroke;
}

void JsonSVGRenderer::SaveGraphic(const GraphicStyle& graphicStyle, json& o)
{
    if (graphicStyle.opacity != 1.0)
        o["opacity"] = graphicStyle.opacity;
    if (graphicStyle.transform)
        o["transform"] = static_cast<JsonSVGTransform*>(graphicStyle.transform.get())->Json();
    if (graphicStyle.clippingPath && graphicStyle.clippingPath->path)
    {
        json clipping;
        if (graphicStyle.clippingPath->clipRule == WindingRule::kEvenOdd)
            clipping["winding"] = "evenodd";
        if (graphicStyle.clippingPath->transform)
            clipping["transform"] = static_cast<JsonSVGTransform*>(graphicStyle.clippingPath->transform.get())->Json();
        clipping["path"] = static_cast<const JsonSVGPath*>(graphicStyle.clippingPath->path.get())->Json();
        o["clipping"] = clipping;
    }
}

void JsonSVGRenderer::SavePaint(const Paint& paint, json& o)
{
    if (paint.type() == typeid(Gradient))
    {
        auto gradient = boost::get<Gradient>(paint);
        json p;
        p["type"] = (gradient.type == GradientType::kLinearGradient ? "linear" : "radial");
        if (gradient.transform)
            p["transform"] = static_cast<JsonSVGTransform*>(gradient.transform.get())->Json();
        if (gradient.type == GradientType::kLinearGradient)
        {
            if (std::isfinite(gradient.x1))
                p["x1"] = gradient.x1;
            if (std::isfinite(gradient.y1))
                p["y1"] = gradient.y1;
            if (std::isfinite(gradient.x2))
                p["x2"] = gradient.x2;
            if (std::isfinite(gradient.y2))
                p["y2"] = gradient.y2;
        }
        else
        {
            if (std::isfinite(gradient.cx))
                p["cx"] = gradient.cx;
            if (std::isfinite(gradient.cy))
                p["cy"] = gradient.cy;
            if (std::isfinite(gradient.fx))
                p["fx"] = gradient.fx;
            if (std::isfinite(gradient.fy))
                p["fy"] = gradient.fy;
        }
        std::string method;
        if (gradient.method == SpreadMethod::kPad)
            method = "pad";
        else if (gradient.method == SpreadMethod::kReflect)
            method = "reflect";
        else if (gradient.method == SpreadMethod::kRepeat)
            method = "repeat";
        if (!method.empty())
            p["method"] = method;

        json stops;
        for (const auto& colorStop : gradient.colorStops)
        {
            const auto& stopColor = colorStop.second;
            stops.push_back({{"offset", colorStop.first}, {"color", stopColor},});
        }
        if (!stops.empty())
            p["stops"] = stops;

        o["paint"]["gradient"] = p;
    }
    else if (paint.type() == typeid(Color))
    {
        auto color = boost::get<Color>(paint);
        if (color != DEFAULT_FILL_COLOR)
            o["paint"]["color"] = color;
    }
}

} // namespace SVGNative
