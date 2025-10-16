#pragma once

#include <string>


class SceneObjectBase
{
public:
    void SetName(const std::string& Name) { m_name = Name; }

    const std::string GetName() const { return m_name; }

protected:
    SceneObjectBase() {}

private:
    std::string m_name = "unnamed_object";
};