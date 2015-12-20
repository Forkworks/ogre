/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include "OgreShaderProgramManager.h"
#include "OgreHighLevelGpuProgramManager.h"
#include "OgreShaderRenderState.h"
#include "OgreShaderProgramSet.h"
#include "OgreShaderProgram.h"
#include "OgreShaderGenerator.h"
#include "OgrePass.h"
#include "OgreLogManager.h"
#include "OgreHighLevelGpuProgram.h"
#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
#include "OgreShaderCGProgramWriter.h"
#include "OgreShaderHLSLProgramWriter.h"
#include "OgreShaderGLSLProgramWriter.h"
#endif
#include "OgreShaderGLSLESProgramWriter.h"
#include "OgreShaderProgramProcessor.h"
#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
#include "OgreShaderCGProgramProcessor.h"
#include "OgreShaderHLSLProgramProcessor.h"
#include "OgreShaderGLSLProgramProcessor.h"
#endif
#include "OgreShaderGLSLESProgramProcessor.h"
#include "OgreGpuProgramManager.h"
#include "Hash\MurmurHash3.h"
#include "OgreIdString.h"


namespace Ogre {

//-----------------------------------------------------------------------
template<> 
RTShader::ProgramManager* Singleton<RTShader::ProgramManager>::msSingleton = 0;

namespace RTShader {


//-----------------------------------------------------------------------
ProgramManager* ProgramManager::getSingletonPtr()
{
    return msSingleton;
}

//-----------------------------------------------------------------------
ProgramManager& ProgramManager::getSingleton()
{
    assert( msSingleton );  
    return ( *msSingleton );
}

//-----------------------------------------------------------------------------
ProgramManager::ProgramManager()
{
    createDefaultProgramProcessors();
    createDefaultProgramWriterFactories();
}

//-----------------------------------------------------------------------------
ProgramManager::~ProgramManager()
{
    flushGpuProgramsCache();
    destroyDefaultProgramWriterFactories();
    destroyDefaultProgramProcessors();  
    destroyProgramWriters();
}

//-----------------------------------------------------------------------------
void ProgramManager::acquirePrograms(Pass* pass, TargetRenderState* renderState)
{
    // Create the CPU programs.
    renderState->createCpuPrograms();


    ProgramSet* programSet = renderState->getProgramSet();

    for (GpuProgramType i = GPT_FIRST; i < GPT_COUNT; ++i)
    {
        Program* p = programSet->getCpuProgram(i);
        if (p != NULL)
        {
            p->setSourcePassName(pass->getFullyQualifiedName());
        }
    }

    // Create the GPU programs.
    createGpuPrograms(programSet);

    // Bind the created GPU programs to the target pass.
    pass->setVertexProgram(programSet->getGpuProgram(GPT_VERTEX_PROGRAM)->getName());
    pass->setFragmentProgram(programSet->getGpuProgram(GPT_FRAGMENT_PROGRAM)->getName());

    bool IsGeometryProgramExists = !programSet->getGpuProgram(GPT_GEOMETRY_PROGRAM).isNull();
    
    
    if (IsGeometryProgramExists)
        pass->setGeometryProgram(programSet->getGpuProgram(GPT_GEOMETRY_PROGRAM)->getName());

    // Bind uniform parameters to pass parameters.
    bindUniformParameters(programSet->getCpuProgram(GPT_VERTEX_PROGRAM), pass->getVertexProgramParameters());
    bindUniformParameters(programSet->getCpuProgram(GPT_FRAGMENT_PROGRAM), pass->getFragmentProgramParameters());

    if (IsGeometryProgramExists)
        bindUniformParameters(programSet->getCpuProgram(GPT_GEOMETRY_PROGRAM), pass->getGeometryProgramParameters());

}

//-----------------------------------------------------------------------------
void ProgramManager::releasePrograms(Pass* pass, TargetRenderState* renderState)
{
    ProgramSet* programSet = renderState->getProgramSet();

    if (programSet != NULL)
    {
        pass->setVertexProgram(BLANKSTRING);
        pass->setFragmentProgram(BLANKSTRING);
        pass->setGeometryProgram(BLANKSTRING);

        for (GpuProgramType i = GPT_FIRST; i < GPT_COUNT; ++i)
        {
            GpuProgramPtr gpuProgram = programSet->getGpuProgram(i);
            if (!gpuProgram.isNull())
            {
                GpuProgramsMap &map = mShaderMap[i];
                GpuProgramsMap::iterator it = map.find(gpuProgram->getName());
                if (it != map.end() && it->second.useCount() == ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS + 1)
                {
                    destroyGpuProgram(it->second);
                    map.erase(it);
                }
            }
        }

        renderState->destroyProgramSet();
    }
}
//-----------------------------------------------------------------------------
void ProgramManager::flushGpuProgramsCache()
{
    for (int i = 0; i < GPT_COUNT; i++)
    {
        flushGpuProgramsCache(mShaderMap[i]);
    }
}

//-----------------------------------------------------------------------------
void ProgramManager::flushGpuProgramsCache(GpuProgramsMap& gpuProgramsMap)
{
    while (gpuProgramsMap.size() > 0)
    {
        GpuProgramsMapIterator it = gpuProgramsMap.begin();

        destroyGpuProgram(it->second);
        gpuProgramsMap.erase(it);
    }
}

//-----------------------------------------------------------------------------
void ProgramManager::createDefaultProgramWriterFactories()
{
    // Add standard shader writer factories 
#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
    mProgramWriterFactories.push_back(OGRE_NEW ShaderProgramWriterCGFactory());
    mProgramWriterFactories.push_back(OGRE_NEW ShaderProgramWriterGLSLFactory());
    mProgramWriterFactories.push_back(OGRE_NEW ShaderProgramWriterHLSLFactory());
#endif
    mProgramWriterFactories.push_back(OGRE_NEW ShaderProgramWriterGLSLESFactory());
    
    for (unsigned int i=0; i < mProgramWriterFactories.size(); ++i)
    {
        ProgramWriterManager::getSingletonPtr()->addFactory(mProgramWriterFactories[i]);
    }
}

//-----------------------------------------------------------------------------
void ProgramManager::destroyDefaultProgramWriterFactories()
{ 
    for (unsigned int i=0; i<mProgramWriterFactories.size(); i++)
    {
        ProgramWriterManager::getSingletonPtr()->removeFactory(mProgramWriterFactories[i]);
        OGRE_DELETE mProgramWriterFactories[i];
    }
    mProgramWriterFactories.clear();
}

//-----------------------------------------------------------------------------
void ProgramManager::createDefaultProgramProcessors()
{
    // Add standard shader processors
#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
    mDefaultProgramProcessors.push_back(OGRE_NEW CGProgramProcessor);
    mDefaultProgramProcessors.push_back(OGRE_NEW GLSLProgramProcessor);
    mDefaultProgramProcessors.push_back(OGRE_NEW HLSLProgramProcessor);
#endif
    mDefaultProgramProcessors.push_back(OGRE_NEW GLSLESProgramProcessor);

    for (unsigned int i=0; i < mDefaultProgramProcessors.size(); ++i)
    {
        addProgramProcessor(mDefaultProgramProcessors[i]);
    }
}

//-----------------------------------------------------------------------------
void ProgramManager::destroyDefaultProgramProcessors()
{
    for (unsigned int i=0; i < mDefaultProgramProcessors.size(); ++i)
    {
        removeProgramProcessor(mDefaultProgramProcessors[i]);
        OGRE_DELETE mDefaultProgramProcessors[i];
    }
    mDefaultProgramProcessors.clear();
}

//-----------------------------------------------------------------------------
void ProgramManager::destroyProgramWriters()
{
    ProgramWriterIterator it    = mProgramWritersMap.begin();
    ProgramWriterIterator itEnd = mProgramWritersMap.end();

    for (; it != itEnd; ++it)
    {
        if (it->second != NULL)
        {
            OGRE_DELETE it->second;
            it->second = NULL;
        }                   
    }
    mProgramWritersMap.clear();
}

//-----------------------------------------------------------------------------
Program* ProgramManager::createCpuProgram(GpuProgramType type)
{
    Program* shaderProgram = OGRE_NEW Program(type);

    mCpuProgramsList.insert(shaderProgram);

    return shaderProgram;
}


//-----------------------------------------------------------------------------
void ProgramManager::destroyCpuProgram(Program* shaderProgram)
{
    ProgramListIterator it    = mCpuProgramsList.find(shaderProgram);
    
    if (it != mCpuProgramsList.end())
    {           
        OGRE_DELETE *it;            
        mCpuProgramsList.erase(it); 
    }           
}

//-----------------------------------------------------------------------------
void ProgramManager::createGpuPrograms(ProgramSet* programSet)
{
    // Before we start we need to make sure that the pixel shader input
    //  parameters are the same as the vertex output, this required by 
    //  shader models 4 and 5.
    // This change may incrase the number of register used in older shader
    //  models - this is why the check is present here.
    bool isVs4 = GpuProgramManager::getSingleton().isSyntaxSupported("vs_4_0_level_9_1");
    if (isVs4)
    {
        synchronizeShaderStagesVariables(programSet);
    }

    // Grab the matching writer.
    const String& language = ShaderGenerator::getSingleton().getTargetLanguage();
    ProgramWriterIterator itWriter = mProgramWritersMap.find(language);
    ProgramWriter* programWriter = NULL;

    // No writer found -> create new one.
    if (itWriter == mProgramWritersMap.end())
    {
        programWriter = ProgramWriterManager::getSingletonPtr()->createProgramWriter(language);
        mProgramWritersMap[language] = programWriter;
    }
    else
    {
        programWriter = itWriter->second;
    }

    ProgramProcessorIterator itProcessor = mProgramProcessorsMap.find(language);
    ProgramProcessor* programProcessor = NULL;

    if (itProcessor == mProgramProcessorsMap.end())
    {
        OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM,
            "Could not find processor for language '" + language,
            "ProgramManager::createGpuPrograms");       
    }

    programProcessor = itProcessor->second;

    bool success;
    
    // Call the pre creation of GPU programs method.
    success = programProcessor->preCreateGpuPrograms(programSet);
    if (success == false)
        OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
            "Could not pre create gpu programs ",
            "ProgramManager::createGpuPrograms");
    
    
    // Create the vertex shader program.
    GpuProgramPtr vsGpuProgram;
    
    vsGpuProgram = createGpuProgram(programSet->getCpuProgram(GPT_VERTEX_PROGRAM), 
        programWriter,
        language, 
        ShaderGenerator::getSingleton().getVertexShaderProfiles(),
        ShaderGenerator::getSingleton().getVertexShaderProfilesList(),
        ShaderGenerator::getSingleton().getShaderCachePath());

    if (vsGpuProgram.isNull())  
        OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
        "Could not create gpu vertex program",
        "ProgramManager::createGpuPrograms");

    programSet->setGpuProgram(GPT_VERTEX_PROGRAM, vsGpuProgram);

    //update flags
    programSet->getGpuProgram(GPT_VERTEX_PROGRAM)->setSkeletalAnimationIncluded(
        programSet->getCpuProgram(GPT_VERTEX_PROGRAM)->getSkeletalAnimationIncluded());

    if (programSet->getCpuProgram(GPT_GEOMETRY_PROGRAM) != NULL)
    {
        GpuProgramPtr gsGpuProgram;

        gsGpuProgram = createGpuProgram(programSet->getCpuProgram(GPT_GEOMETRY_PROGRAM),
            programWriter,
            language,
            ShaderGenerator::getSingleton().getGeometryShaderProfiles(),
            ShaderGenerator::getSingleton().getGeometryShaderProfilesList(),
            ShaderGenerator::getSingleton().getShaderCachePath());

        if (gsGpuProgram.isNull())
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
            "Could not create gpu geomerty program",
            "ProgramManager::createGpuPrograms");

        programSet->setGpuProgram(GPT_GEOMETRY_PROGRAM, gsGpuProgram);
    }
    // Create the fragment shader program.
    GpuProgramPtr psGpuProgram;

    psGpuProgram = createGpuProgram(programSet->getCpuProgram(GPT_FRAGMENT_PROGRAM), 
        programWriter,
        language, 
        ShaderGenerator::getSingleton().getFragmentShaderProfiles(),
        ShaderGenerator::getSingleton().getFragmentShaderProfilesList(),
        ShaderGenerator::getSingleton().getShaderCachePath());

    if (psGpuProgram.isNull())  
        OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
            "Could not create gpu fragment program",
            "ProgramManager::createGpuPrograms");


    programSet->setGpuProgram(GPT_FRAGMENT_PROGRAM, psGpuProgram);

    // Call the post creation of GPU programs method.
    success = programProcessor->postCreateGpuPrograms(programSet);
    if (success == false)   
        OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
        "Could not post process gpu programs",
        "ProgramManager::createGpuPrograms");


}


//-----------------------------------------------------------------------------
void ProgramManager::bindUniformParameters(Program* pCpuProgram, const GpuProgramParametersSharedPtr& passParams)
{
    const UniformParameterList& progParams = pCpuProgram->getParameters();
    UniformParameterConstIterator itParams = progParams.begin();
    UniformParameterConstIterator itParamsEnd = progParams.end();

    // Bind each uniform parameter to its GPU parameter.
    for (; itParams != itParamsEnd; ++itParams)
    {           
        (*itParams)->bind(passParams);                  
    }
}

//-----------------------------------------------------------------------------
GpuProgramPtr ProgramManager::createGpuProgram(Program* shaderProgram, 
                                               ProgramWriter* programWriter,
                                               const String& language,
                                               const String& profiles,
                                               const StringVector& profilesList,
                                               const String& cachePath)
{
    stringstream sourceCodeStringStream;
    String programName;

    // Generate source code.
    programWriter->writeSourceCode(sourceCodeStringStream, shaderProgram);
    String source = sourceCodeStringStream.str();

#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
    
    // Generate program name.
    String::size_type idx = source.find(ProgramWriter::msProgramStartToken);
    
    if (idx != String::npos)
    {
        String sourceWithoutHeader = source.substr(idx, source.length() - idx);
        programName = generateGUID(sourceWithoutHeader);
    }
    else
    {
        OGRE_EXCEPT(Exception::ERR_INVALID_STATE,"Wrong program source detected",
            "ProgramManager::createGpuProgram");
    }

#else // Disable caching on android devices 

    // Generate program name.
    static int gpuProgramID = 0;
    programName = "RTSS_"  + StringConverter::toString(++gpuProgramID);
   
#endif
    
    GpuProgramType type = shaderProgram->getType();
    switch (type)
    {
    case GPT_VERTEX_PROGRAM:
        programName += "_VS";
        break;
    case GPT_FRAGMENT_PROGRAM:
        programName += "_FS";
        break;
    case GPT_GEOMETRY_PROGRAM:
        programName += "_GS";
        break;
    }

    
    // Try to get program by name.
    HighLevelGpuProgramPtr pGpuProgram = HighLevelGpuProgramManager::getSingleton().getByName(programName);

    // Case the program doesn't exist yet.
    if (pGpuProgram.isNull())
    {
        // Create new GPU program.
        pGpuProgram = HighLevelGpuProgramManager::getSingleton().createProgram(programName,
            ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, language, shaderProgram->getType());

        // Case cache directory specified -> create program from file.
        if (cachePath.empty() == false)
        {
            const String  programFullName = programName + "." + language;
            const String  programFileName = cachePath + programFullName;    
            std::ifstream programFile;
            bool          writeFile = true;


            // Check if program file already exist.
            programFile.open(programFileName.c_str());

            // Case no matching file found -> we have to write it.
            if (!programFile)
            {           
                writeFile = true;
            }
            else
            {
                writeFile = false;
                programFile.close();
            }

            // Case we have to write the program to a file.
            if (writeFile)
            {
                std::ofstream outFile(programFileName.c_str());

                if (!outFile)
                    return GpuProgramPtr();

                outFile << source;
                outFile.close();
            }

            pGpuProgram->setSourceFile(programFullName);
        }

        // No cache directory specified -> create program from system memory.
        else
        {
            pGpuProgram->setSource(source);
        }
        
        
        pGpuProgram->setParameter("entry_point", shaderProgram->getEntryPointFunction()->getName());

        if (language == "hlsl")
        {
            // HLSL program requires specific target profile settings - we have to split the profile string.
            StringVector::const_iterator it = profilesList.begin();
            StringVector::const_iterator itEnd = profilesList.end();
            
            for (; it != itEnd; ++it)
            {
                if (GpuProgramManager::getSingleton().isSyntaxSupported(*it))
                {
                    pGpuProgram->setParameter("target", *it);
                    break;
                }
            }

            pGpuProgram->setParameter("enable_backwards_compatibility", "false");
            pGpuProgram->setParameter("column_major_matrices", StringConverter::toString(shaderProgram->getUseColumnMajorMatrices()));
        }
        
        pGpuProgram->setParameter("profiles", profiles);
        pGpuProgram->load();
    
        // Case an error occurred.
        if (pGpuProgram->hasCompileError())
        {
            StringStream ss;
            ss << "RTSS has generated a gpu program from render states that caused a compilation error.\n"
               << pGpuProgram->getCompileErrorMessage();
        
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,ss.str(),
                        "ProgramManager::acquireGpuPrograms");
        }

        // Add the created GPU program to local cache.
        mShaderMap[pGpuProgram->getType()][programName] = pGpuProgram;
    }
    
    return GpuProgramPtr(pGpuProgram);
}


//-----------------------------------------------------------------------------
String ProgramManager::generateGUID(const String& programString)
{
    // Murmur3 128 bit is used to avoid collisions.
    // Different programs must have unique hash values. Some programs only differ in the size of array parameters.

    const void* dataPtr = (const void*)programString.c_str();
    const size_t size = programString.size();
    
    unsigned int hashValue[4];

    MurmurHash3_128(dataPtr, size, IdString::Seed, &hashValue[0]);
    
    //Generate the guid string
    stringstream stream;
    stream.fill('0');
    stream.setf(std::ios::fixed);
    stream.setf(std::ios::hex, std::ios::basefield);
    stream.width(8); stream << hashValue[0] << "-";
    stream.width(4); stream << (uint16)(hashValue[1] >> 16) << "-";
    stream.width(4); stream << (uint16)(hashValue[1]) << "-";
    stream.width(4); stream << (uint16)(hashValue[2] >> 16) << "-";
    stream.width(4); stream << (uint16)(hashValue[2]);
    stream.width(8); stream << hashValue[3];
    return stream.str();
}


//-----------------------------------------------------------------------------
void ProgramManager::addProgramProcessor(ProgramProcessor* processor)
{
    
    ProgramProcessorIterator itFind = mProgramProcessorsMap.find(processor->getTargetLanguage());

    if (itFind != mProgramProcessorsMap.end())
    {
        OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM,
            "A processor for language '" + processor->getTargetLanguage() + "' already exists.",
            "ProgramManager::addProgramProcessor");
    }       

    mProgramProcessorsMap[processor->getTargetLanguage()] = processor;
}

//-----------------------------------------------------------------------------
void ProgramManager::removeProgramProcessor(ProgramProcessor* processor)
{
    ProgramProcessorIterator itFind = mProgramProcessorsMap.find(processor->getTargetLanguage());

    if (itFind != mProgramProcessorsMap.end())
        mProgramProcessorsMap.erase(itFind);

}

//-----------------------------------------------------------------------------
void ProgramManager::destroyGpuProgram(GpuProgramPtr& gpuProgram)
{       
    const String& programName = gpuProgram->getName();
    ResourcePtr res           = HighLevelGpuProgramManager::getSingleton().getByName(programName);  

    if (res.isNull() == false)
    {       
        HighLevelGpuProgramManager::getSingleton().remove(programName);
    }
}

//-----------------------------------------------------------------------
void ProgramManager::synchronizeShaderStagesVariables( ProgramSet* programSet )
{
    Function* vertexMain = programSet->getCpuProgram(GPT_VERTEX_PROGRAM)->getEntryPointFunction();
    Function* pixelMain = programSet->getCpuProgram(GPT_FRAGMENT_PROGRAM)->getEntryPointFunction();

    
    Program* gsProgram = programSet->getCpuProgram(GPT_GEOMETRY_PROGRAM);
    Function* geometryMain = gsProgram == NULL ? NULL : gsProgram->getEntryPointFunction();

    if (geometryMain != NULL)
    {
        geometryMain->synchronizeInputParamsTo(vertexMain);
        pixelMain->synchronizeInputParamsTo(geometryMain);
    }
    else
    {
        pixelMain->synchronizeInputParamsTo(vertexMain);
    }
}
//-----------------------------------------------------------------------
size_t ProgramManager::getShaderCount(GpuProgramType programType) const
{
    return mShaderMap[programType].size();
}
//-----------------------------------------------------------------------
size_t ProgramManager::getFragmentShaderCount()  const
{
    return getShaderCount(GPT_FRAGMENT_PROGRAM);
}
//-----------------------------------------------------------------------
size_t ProgramManager::getVertexShaderCount()  const
{
    return getShaderCount(GPT_VERTEX_PROGRAM);
}

/** @} */
/** @} */
}
}
