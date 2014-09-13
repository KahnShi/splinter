/*
This file is part of the Multivariate Splines library.
Copyright (C) 2012 Bjarne Grimstad (bjarne.grimstad@gmail.com)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "datatable.h"

#include <string>
#include <fstream>
#include <stdexcept>
#include <limits>

namespace MultivariateSplines
{

//Simple definition of checked strto* functions according to the implementations of sto* C++11 functions at:
//  http://msdn.microsoft.com/en-us/library/ee404775.aspx
//  http://msdn.microsoft.com/en-us/library/ee404860.aspx
//  https://gcc.gnu.org/svn/gcc/trunk/libstdc++-v3/include/bits/basic_string.h
//  https://gcc.gnu.org/svn/gcc/trunk/libstdc++-v3/include/ext/string_conversions.h

double checked_strtod(const char* _Str, char** _Eptr) {

    double _Ret;
    char* _EptrTmp;

    errno = 0;

    _Ret = std::strtod(_Str, &_EptrTmp);

    if(_EptrTmp == _Str)
    {
        throw std::invalid_argument("strtod");
    }
    else if(errno == ERANGE)
    {
        throw std::out_of_range("strtod");
    }
    else
    {
        if(_Eptr != nullptr)
        {
            *_Eptr = _EptrTmp;
        }

        return _Ret;
    }
}

int checked_strtol(const char* _Str, char** _Eptr, size_t _Base = 10) {

    long _Ret;
    char* _EptrTmp;

    errno = 0;

    _Ret = std::strtol(_Str, &_EptrTmp, _Base);

    if(_EptrTmp == _Str)
    {
        throw std::invalid_argument("strtol");
    }
    else if(errno == ERANGE ||
            (_Ret < std::numeric_limits<int>::min() || _Ret > std::numeric_limits<int>::max()))
    {
        throw std::out_of_range("strtol");
    }
    else
    {
        if(_Eptr != nullptr)
        {
            *_Eptr = _EptrTmp;
        }

        return _Ret;
    }
}


DataTable::DataTable()
    : DataTable(false, false)
{
}

DataTable::DataTable(bool allowDuplicates)
    : DataTable(allowDuplicates, false)
{
}

DataTable::DataTable(bool allowDuplicates, bool allowIncompleteGrid)
    : allowDuplicates(allowDuplicates),
      allowIncompleteGrid(allowIncompleteGrid),
      numDuplicates(0),
      numVariables(0)
{
}

void DataTable::addSample(double x, double y)
{
    addSample(DataSample(x, y));
}

void DataTable::addSample(std::vector<double> x, double y)
{
    addSample(DataSample(x, y));
}

void DataTable::addSample(DenseVector x, double y)
{
    addSample(DataSample(x, y));
}

void DataTable::addSample(const DataSample &sample)
{
    if(getNumSamples() == 0)
    {
        numVariables = sample.getDimX();
        initDataStructures();
    }

    assert(sample.getDimX() == numVariables); // All points must have the same dimension

    // Check if the sample has been added already
    if(samples.count(sample) > 0)
    {
        if(!allowDuplicates)
        {
            std::cout << "Discarding duplicate sample because allowDuplicates is false!" << std::endl;
            std::cout << "Initialise with DataTable(true) to set it to true." << std::endl;
            return;
        }

        numDuplicates++;
    }

    samples.insert(sample);

    recordGridPoint(sample);
}

void DataTable::recordGridPoint(const DataSample &sample)
{
    for(unsigned int i = 0; i < getNumVariables(); i++)
    {
        grid.at(i).insert(sample.getX().at(i));
    }
}

unsigned int DataTable::getNumSamplesRequired() const
{
    unsigned long samplesRequired = 1;
    unsigned int i = 0;
    for(auto &variable : grid)
    {
        samplesRequired *= (unsigned long) variable.size();
        i++;
    }

    return (i > 0 ? samplesRequired : (unsigned long) 0);
}

bool DataTable::isGridComplete() const
{
    return samples.size() > 0 && samples.size() - numDuplicates == getNumSamplesRequired();
}

void DataTable::initDataStructures()
{
    for(unsigned int i = 0; i < getNumVariables(); i++)
    {
        grid.push_back(std::set<double>());
    }
}

void DataTable::gridCompleteGuard() const
{
    if(!isGridComplete() && !allowIncompleteGrid)
    {
        std::cout << "The grid is not complete yet!" << std::endl;
        exit(1);
    }
}

/***********
 * Getters *
 ***********/

std::multiset<DataSample>::const_iterator DataTable::cbegin() const
{
    gridCompleteGuard();

    return samples.cbegin();
}

std::multiset<DataSample>::const_iterator DataTable::cend() const
{
    gridCompleteGuard();

    return samples.cend();
}

// Get table of samples x-values,
// i.e. table[i][j] is the value of variable i at sample j
std::vector< std::vector<double> > DataTable::getTableX() const
{
    gridCompleteGuard();

    // Initialize table
    std::vector<std::vector<double>> table;
    for(unsigned int i = 0; i < numVariables; i++)
    {
        std::vector<double> xi(getNumSamples(), 0.0);
        table.push_back(xi);
    }

    // Fill table with values
    int i = 0;
    for(auto &sample : samples)
    {
        std::vector<double> x = sample.getX();

        for(unsigned int j = 0; j < numVariables; j++)
        {
            table.at(j).at(i) = x.at(j);
        }
        i++;
    }

    return table;
}

// Get vector of y-values
std::vector<double> DataTable::getVectorY() const
{
    std::vector<double> y;
    for(std::multiset<DataSample>::const_iterator it = cbegin(); it != cend(); ++it)
    {
        y.push_back(it->getY());
    }
    return y;
}

/*****************
 * Save and load *
 *****************/

void DataTable::save(std::string fileName) const
{
    std::ofstream outFile;

    try
    {
        outFile.open(fileName);
    }
    catch(const std::ios_base::failure &e)
    {
        throw;
    }

    // If this function is still alive the file must be open,
    // no need to call is_open()

    // Write header
    outFile << "# Saved DataTable" << '\n';
    outFile << "# Number of samples: " << getNumSamples() << '\n';
    outFile << "# Complete grid: " << (isGridComplete() ? "yes" : "no") << '\n';
    outFile << "# xDim: " << numVariables << '\n';
    outFile << numVariables << " " << 1 << '\n';

    for(auto it = cbegin(); it != cend(); it++)
    {
        for(unsigned int i = 0; i < numVariables; i++)
        {
            outFile << std::setprecision(SAVE_DOUBLE_PRECISION) << it->getX().at(i) << " ";
        }

        outFile << std::setprecision(SAVE_DOUBLE_PRECISION) << it->getY();

        outFile << '\n';
    }

    // close() also flushes
    try
    {
        outFile.close();
    }
    catch(const std::ios_base::failure &e)
    {
        throw;
    }
}

void DataTable::load(std::string fileName)
{
    std::ifstream inFile;

    try
    {
        inFile.open(fileName);
    }
    catch(const std::ios_base::failure &e)
    {
        throw;
    }

    // If this function is still alive the file must be open,
    // no need to call is_open()

    // Skip past comments
    std::string line;

    int nX, nY;
    int state = 0;
    while(std::getline(inFile, line))
    {
        // Look for comment sign
        if(line.at(0) == '#')
            continue;

        // Reading number of dimensions
        if(state == 0)
        {
            nX = checked_strtol(line.c_str(), nullptr, 10);
            nY = 1;
            state = 1;
        }

        // Reading samples
        else if(state == 1)
        {
            auto x = std::vector<double>(nX);
            auto y = std::vector<double>(nY);

            const char* str = line.c_str();
            char* nextStr = nullptr;

            for(int i = 0; i < nX; i++)
            {
                x.at(i) = checked_strtod(str, &nextStr);
                str = nextStr;
            }
            for(int j = 0; j < nY; j++)
            {
                y.at(j) = checked_strtod(str, &nextStr);
                str = nextStr;
            }

            addSample(x, y.at(0));
        }
    }

    // close() also flushes
    try
    {
        inFile.close();
    }
    catch(const std::ios_base::failure &e)
    {
        throw;
    }
}

/**************
 * Debug code *
 **************/
void DataTable::printSamples() const
{
    for(auto &sample : samples)
    {
        std::cout << sample << std::endl;
    }
}

void DataTable::printGrid() const
{
    std::cout << "===== Printing grid =====" << std::endl;

    unsigned int i = 0;
    for(auto &variable : grid)
    {
        std::cout << "x" << i++ << "(" << variable.size() << "): ";
        for(double value : variable)
        {
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "Unique samples added: " << samples.size() << std::endl;
    std::cout << "Samples required: " << getNumSamplesRequired() << std::endl;
}

} // namespace MultivariateSplines
