#include "element.h"
#include "parser.h"
#include "svgelement.h"

namespace lunasvg {

static const std::string EmptyString;
static const std::string InheritString{"inherit"};

Property::Property(PropertyId id, const std::string& value, int specificity)
    : id(id), value(value), specificity(specificity)
{
}

void PropertyList::set(PropertyId id, const std::string& value, int specificity)
{
    auto property = get(id);
    if(property == nullptr)
    {
        m_properties.emplace_back(id, value, specificity);
        return;
    }

    if(property->specificity > specificity)
        return;

    property->specificity = specificity;
    property->value = value;
}

Property* PropertyList::get(PropertyId id) const
{
    auto size = m_properties.size();
    for(std::size_t i = 0;i < size;i++)
    {
        auto& property = m_properties[i];
        if(property.id == id)
            return const_cast<Property*>(&property);
    }

    return nullptr;
}

void PropertyList::add(const Property& property)
{
    set(property.id, property.value, property.specificity);
}

void PropertyList::add(const PropertyList& properties)
{
    auto it = properties.m_properties.begin();
    auto end = properties.m_properties.end();
    for(;it != end;++it)
        add(*it);
}

Element::Element(ElementId id)
    : id(id)
{
}

void Element::set(PropertyId id, const std::string& value, int specificity)
{
    properties.set(id, value, specificity);
}

const std::string& Element::get(PropertyId id) const
{
    auto property = properties.get(id);
    if(property == nullptr)
        return EmptyString;

    return property->value;
}

const std::string& Element::find(PropertyId id) const
{
    auto element = this;
    while(element)
    {
        auto& value = element->get(id);
        if(!value.empty() && value != InheritString)
            return value;
        element = element->parent;
    }

    return EmptyString;
}

bool Element::has(PropertyId id) const
{
    return properties.get(id);
}

Element* Element::previousSibling() const
{
    if(parent == nullptr)
        return nullptr;

    Element* element = nullptr;
    auto it = parent->children.begin();
    auto end = parent->children.end();
    for(;it != end;++it)
    {
        auto node = it->get();
        if(node->isText())
            continue;

        if(node == this)
            return element;
        element = static_cast<Element*>(node);
    }

    return nullptr;
}

Element* Element::nextSibling() const
{
    if(parent == nullptr)
        return nullptr;

    Element* element = nullptr;
    auto it = parent->children.rbegin();
    auto end = parent->children.rend();
    for(;it != end;++it)
    {
        auto node = it->get();
        if(node->isText())
            continue;

        if(node == this)
            return element;
        element = static_cast<Element*>(node);
    }

    return nullptr;
}

Node* Element::addChild(std::unique_ptr<Node> child)
{
    child->parent = this;
    children.push_back(std::move(child));
    return &*children.back();
}

Rect Element::nearestViewBox() const
{
    if(parent == nullptr)
        return Rect{0, 0, 512, 512};

    if(parent->id == ElementId::Svg)
    {
        auto element = static_cast<SVGElement*>(parent);
        if(element->has(PropertyId::ViewBox))
            return element->viewBox();

        LengthContext lengthContext(this);
        auto _x = lengthContext.valueForLength(element->x(), LengthMode::Width);
        auto _y = lengthContext.valueForLength(element->y(), LengthMode::Height);
        auto _w = lengthContext.valueForLength(element->width(), LengthMode::Width);
        auto _h = lengthContext.valueForLength(element->height(), LengthMode::Height);
        return Rect{_x, _y, _w, _h};
    }

    return parent->nearestViewBox();
}

void Element::layoutChildren(LayoutContext* context, LayoutContainer* current) const
{
    for(auto& child : children)
        child->layout(context, current);
}

void Node::layout(LayoutContext*, LayoutContainer*) const
{
}

std::unique_ptr<Node> TextNode::clone() const
{
    auto node = std::make_unique<TextNode>();
    node->text = text;
    return std::move(node);
}

} // namespace lunasvg
