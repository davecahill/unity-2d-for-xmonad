#include "inputshaperectangle.h"
#include "inputshapemask.h"

#include <QBitmap>
#include <QPainter>
#include <QPainterPath>
#include <QDebug>
#include <QRect>

InputShapeRectangle::InputShapeRectangle(QObject *parent) :
    QObject(parent),
    m_enabled(true)
{
}

void InputShapeRectangle::updateShape()
{
    QBitmap newShape(m_rectangle.width(), m_rectangle.height());
    newShape.fill(Qt::color1);

    if (!m_rectangle.isEmpty() && m_masks.count() > 0) {
        QPainter painter(&newShape);
        painter.setBackgroundMode(Qt::OpaqueMode);

        Q_FOREACH (InputShapeMask* mask, m_masks) {
            if (mask->enabled()) {
                painter.drawPixmap(mask->position(), mask->shape());
            }
        }
    }

    m_shape = newShape;
    Q_EMIT shapeChanged();
}

QRect InputShapeRectangle::rectangle() const
{
    return m_rectangle;
}

void InputShapeRectangle::setRectangle(QRect rectangle)
{
    if (rectangle != m_rectangle) {
        m_rectangle = rectangle;
        updateShape();
        Q_EMIT rectangleChanged();
    }
}

bool InputShapeRectangle::enabled() const
{
    return m_enabled;
}

void InputShapeRectangle::setEnabled(bool enabled)
{
    if (enabled != m_enabled) {
        m_enabled = enabled;
        Q_EMIT enabledChanged();

    }
}

QBitmap InputShapeRectangle::shape() const
{
    return m_shape;
}

QDeclarativeListProperty<InputShapeMask> InputShapeRectangle::masks()
{
    return QDeclarativeListProperty<InputShapeMask>(this, this, &InputShapeRectangle::appendMask);
}

void InputShapeRectangle::appendMask(QDeclarativeListProperty<InputShapeMask> *list, InputShapeMask *mask)
{
    InputShapeRectangle* instance = qobject_cast<InputShapeRectangle*>(list->object);
    if (instance != NULL) {
        instance->m_masks.append(mask);
        instance->connect(mask, SIGNAL(enabledChanged()), SLOT(updateShape()));
        instance->connect(mask, SIGNAL(shapeChanged()), SLOT(updateShape()));
        instance->connect(mask, SIGNAL(positionChanged()), SLOT(updateShape()));
        instance->updateShape();
    }
}

#include "inputshaperectangle.moc"
