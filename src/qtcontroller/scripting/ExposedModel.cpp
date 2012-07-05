#include <tinia/qtcontroller/scripting/ExposedModel.hpp>

namespace tinia {
namespace qtcontroller {
namespace scripting {


ExposedModel::ExposedModel(std::shared_ptr<tinia::model::ExposedModel> model,
                           QScriptEngine *engine,
                           QObject *parent)
    : QObject(parent), m_engine(engine), m_model(model)
{
}

void ExposedModel::updateElement(const QString &key, QScriptValue value)
{
    auto schemaElement = m_model->getStateSchemaElement(key.toStdString());
    auto type = schemaElement.getXSDType();

    if(type.find("xsd:") != std::string::npos) {
        type = type.substr(4);
    }


    if (type == std::string("double")) {
        m_model->updateElement(key.toStdString(), double(value.toNumber()));
    }
    else if(type==std::string("integer"))  {
        m_model->updateElement(key.toStdString(), int(value.toNumber()));
    }
    else if(type==std::string("bool")) {
        m_model->updateElement(key.toStdString(), value.toBool());
    }
    else if(type==std::string("string")) {
        m_model->updateElement(key.toStdString(), value.toString().toStdString());
    }
    else if(type==std::string("complexType")) {
        m_model->updateElement(key.toStdString(), static_cast<Viewer*>(value.toQObject())->viewer());
    }

}


QScriptValue ExposedModel::getElementValue(const QString &key)
{
    auto schemaElement = m_model->getStateSchemaElement(key.toStdString());
    auto type = schemaElement.getXSDType();
    if(type.find("xsd:") != std::string::npos) {
        type = type.substr(4);
    }

    if (type == std::string("double")) {
        double value;
        m_model->getElementValue(key.toStdString(), value);
        return QScriptValue(value);
    }
    if (type == std::string("integer")) {
        int value;
        m_model->getElementValue(key.toStdString(), value);
        return QScriptValue(value);
    }
    if (type == std::string("bool")) {
        bool value;
        m_model->getElementValue(key.toStdString(), value);
        return QScriptValue(value);
    }
    if (type == std::string("string")) {
        std::string value;
        m_model->getElementValue(key.toStdString(), value);
        return QScriptValue(QString(value.c_str()));
    }
    if (type == std::string("complexType")) {
        auto v = new Viewer(m_engine, this);
        m_model->getElementValue(key.toStdString(), v->viewer());
        return m_engine->newQObject(v);
    }
    return QScriptValue();
}

} // namespace scripting
} // namespace qtcontroller
} // namespace tinia
