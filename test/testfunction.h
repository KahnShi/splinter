/*
 * This file is part of the SPLINTER library.
 * Copyright (C) 2012 Bjarne Grimstad (bjarne.grimstad@gmail.com).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef SPLINTER_TESTFUNCTION_H
#define SPLINTER_TESTFUNCTION_H

#include "function.h"
#include <vector>

namespace SPLINTER
{

class TestFunction : public Function
{
public:
    TestFunction(std::function<double (const std::vector<double> &)> f, size_t numVariables,
                 std::string functionString);
    TestFunction(std::function<double (const std::vector<double> &)> f, size_t numVariables,
                 std::string functionString, std::vector<double> &degrees);

    virtual ~TestFunction();

    /**
     * Avoid name hiding.
     * Overriding a method that is overloaded causes all methods with the same name
     * to be hidden. Doing this avoids that "problem" (it is actually a feature).
     */
    using Function::eval;
    double eval(const std::vector<double> &x) const override;

    inline std::string getFunctionStr() const { return functionString; }

    inline bool isConstDegree() const { return constDegree; }
    inline std::vector<double> getConstDegree() const { return constDegreeVal; }
    std::vector<unsigned int> getConstDegreeInt() const;

    double getMaxDegree() const;

    void save(const std::string fileName) const override {}


private:
    std::string functionString;

    bool constDegree;
    std::vector<double> constDegreeVal;

    std::function<double (const std::vector<double> &)> f;

    void load(const std::string fileName) override {}
};

} // namespace SPLINTER

#endif // SPLINTER_TESTFUNCTION_H
