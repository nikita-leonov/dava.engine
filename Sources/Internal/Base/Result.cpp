/*==================================================================================
    Copyright (c) 2008, binaryzebra
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the binaryzebra nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/


#include "result.h"

using namespace DAVA;

Result::Result(ResultType type_, const DAVA::String &text, const DAVA::VariantType &data_)
    : type(type_)
    , resultText(text)
    , data(data_)
{
}

ResultList::ResultList()
    : allOk(true)
{
    
}

ResultList::ResultList(const Result& result)
    : allOk(result)
{
    
}

ResultList& ResultList::AddResult(const Result &result)
{
    allOk &= result;
    results.push_back(result);
    return *this;
}

ResultList& ResultList::AddResult(const Result::ResultType type, const String &error, const VariantType &data)
{
    Result result(type, error, data);
    return AddResult(result);
}

List<Result::ResultType> ResultList::GetResultTypes() const
{
    List<Result::ResultType> resultTypes;
    for(const auto &result : results)
    {
        resultTypes.push_back(result.type);
    }
    return resultTypes;
}

List<String> ResultList::GetErrors() const
{
    List<String> errors;
    for(const auto &result : results)
    {
        errors.push_back(result.resultText);
    }
    return errors;
}