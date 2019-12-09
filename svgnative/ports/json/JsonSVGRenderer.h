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

#ifndef SVGViewer_JsonSVGRenderer_h
#define SVGViewer_JsonSVGRenderer_h

#include "SVGRenderer.h"

#include <stack>
#include <string>

#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;

namespace SVGNative
{
class JsonSVGPath final : public Path
{
public:
    JsonSVGPath();

    void Rect(float x, float y, float width, float height) override;
    void RoundedRect(float x, float y, float width, float height, float cornerRadius) override;
    void Ellipse(float cx, float cy, float rx, float ry) override;

    void MoveTo(float x, float y) override;
    void LineTo(float x, float y) override;
    void CurveTo(float x1, float y1, float x2, float y2, float x3, float y3) override;
    void CurveToV(float x2, float y2, float x3, float y3) override;
    void ClosePath() override;

    json Json() const { return j; }

private:
    json j;
};

class JsonSVGTransform final : public Transform
{
public:
    struct AffineTransform
    {
        AffineTransform() = default;
        AffineTransform(float aA, float aB, float aC, float aD, float aE, float aF)
            : a{aA}
            , b{aB}
            , c{aC}
            , d{aD}
            , e{aE}
            , f{aF}
        {
        }
        float a{1};
        float b{0};
        float c{0};
        float d{1};
        float e{0};
        float f{0};
    };

    JsonSVGTransform(float a, float b, float c, float d, float tx, float ty);

    void Set(float a, float b, float c, float d, float tx, float ty) override;
    void Rotate(float r) override;
    void Translate(float tx, float ty) override;
    void Scale(float sx, float sy) override;
    void Concat(const Transform& other) override;

    json Json() const;

private:
    void Multiply(const AffineTransform& o);

private:
    AffineTransform mTransform{};
};

class JsonSVGImageData final : public ImageData
{
public:
    JsonSVGImageData(const std::string& base64, ImageEncoding encoding)
        : mBase64{base64}, mEncoding{encoding} {}

    // We are not able to encode PNG images here so we return a fixed size.
    float Width() const override { return 160.0f; }

    float Height() const override { return 110.0f; }

    json Json() const;

private:
    std::string mBase64;
    ImageEncoding mEncoding;
};

class JsonSVGRenderer final : public SVGRenderer
{
public:
    JsonSVGRenderer();

    std::unique_ptr<ImageData> CreateImageData(const std::string& base64, ImageEncoding encoding) override { return std::unique_ptr<JsonSVGImageData>(new JsonSVGImageData(base64, encoding)); }

    std::unique_ptr<Path> CreatePath() override { return std::unique_ptr<JsonSVGPath>(new JsonSVGPath); }

    std::unique_ptr<Transform> CreateTransform(
        float a = 1.0, float b = 0.0, float c = 0.0, float d = 1.0, float tx = 0.0, float ty = 0.0) override
    {
        return std::unique_ptr<JsonSVGTransform>(new JsonSVGTransform(a, b, c, d, tx, ty));
    }

    void Save(const GraphicStyle& graphicStyle) override;
    void Restore() override;

    void DrawPath(const Path& path, const GraphicStyle& graphicStyle, const FillStyle& fillStyle, const StrokeStyle& strokeStyle) override;
    void DrawImage(const ImageData& image, const GraphicStyle& graphicStyle, const Rect& clipArea, const Rect& fillArea) override;

    std::string String() const;
    json Json() const;

private:
    void SaveFill(const FillStyle& fillStyle, json& o);
    void SaveStroke(const StrokeStyle& strokeStyle, json& o);
    void SaveGraphic(const GraphicStyle& graphicStyle, json& o);
    void SavePaint(const Paint& paint, json& o);

    json mRoot;
    std::stack<json*> mGroupStack;
    json* mCurrentGroup;
};

} // namespace SVGNative

#endif // SVGViewer_JsonSVGRenderer_h