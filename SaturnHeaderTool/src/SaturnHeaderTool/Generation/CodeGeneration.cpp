/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2025 BEAST                                                           *
*                                                                                           *
* Permission is hereby granted, free of charge, to any person obtaining a copy              *
* of this software and associated documentation files (the "Software"), to deal             *
* in the Software without restriction, including without limitation the rights              *
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell                 *
* copies of the Software, and to permit persons to whom the Software is                     *
* furnished to do so, subject to the following conditions:                                  *
*                                                                                           *
* The above copyright notice and this permission notice shall be included in all            *
* copies or substantial portions of the Software.                                           *
*                                                                                           *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR                *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,                  *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE               *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER                    *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,             *
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE             *
* SOFTWARE.                                                                                 *
*********************************************************************************************
*/

#include "CodeGeneration.h"
#include "SaturnHeaderTool/Base/Errors.h"

#include "Saturn/GameFramework/SClass.h"

#include <regex>
#include <fstream>
#include <iostream>

namespace Saturn {

	HeaderTool::HeaderTool()
	{
	}

	HeaderTool::~HeaderTool()
	{
		m_Commands.clear();
	}

	void HeaderTool::SetWorkingDir( const std::filesystem::path& rPath )
	{
		m_WorkingDir = rPath;
	}

	void HeaderTool::SubmitWorkList( const std::vector<std::filesystem::path>& rCommands )
	{
		m_Commands.reserve( rCommands.size() );

		for( const auto& rCommand : rCommands )
		{
			HeaderToolCommand command;
			command.Filepath = rCommand;

			m_Commands.push_back( command );
		}
	}

	bool HeaderTool::StartGeneration()
	{
		int tasksFailed = 0;

		for( auto& rCommand : m_Commands )
		{
			if( !GenerateHeader( rCommand ) )
			{
				tasksFailed++;
				continue;
			}

			if( !GenerateSource( rCommand ) ) tasksFailed++;
		}

		return tasksFailed == 0;
	}

	[[nodiscard]] static bool LineIsNotComment( const std::string& rLine )
	{
		std::regex regex( R"(^\s*(//.*|/\*.*\*/|/\*.*))" );
		return !std::regex_match( rLine, regex );
	}

	static std::pair<std::string, std::string> GetClassNameAndBaseClass( const std::string& rLine )
	{
		if( !LineIsNotComment( rLine ) )
			return {};

		std::regex classPattern( R"(class\s+(\w+)\s*:\s*public\s+(\w+))" );
		std::smatch match;

		if( std::regex_search( rLine, match, classPattern ) )
		{
			return std::make_pair( match[ 1 ].str(), match[ 2 ].str() );
		}

		return {};
	}

	static bool IsValidPointer( const std::string& rString ) 
	{
		std::regex pattern( R"(^\s*[a-zA-Z_][a-zA-Z0-9_]*(::[a-zA-Z_][a-zA-Z0-9_]*)*\s*\*\s*$)" );
		return std::regex_match( rString, pattern );
	}

	static SPropertyType StringToSPropertyType( const std::string& str, bool usingNamespace )
	{
		if( usingNamespace )
		{
			if( str == "char" )             return SPropertyType::Char;
			else if( str == "float" )       return SPropertyType::Float;
			else if( str == "int" )         return SPropertyType::Int;
			else if( str == "double" )      return SPropertyType::Double;
			else if( str == "uint8_t" )     return SPropertyType::Uint8;
			else if( str == "uint16_t" )    return SPropertyType::Uint16;
			else if( str == "uint32_t" )    return SPropertyType::Uint32;
			else if( str == "uint64_t" )    return SPropertyType::Uint64;
			else if( str == "int8_t" )      return SPropertyType::Int8;
			else if( str == "int16_t" )     return SPropertyType::Int16;
			else if( str == "int64_t" )     return SPropertyType::Int64;
			else if( str == "glm::vec2" )   return SPropertyType::Vector2;
			else if( str == "glm::vec3" )   return SPropertyType::Vector3;
			else if( str == "glm::vec4" )   return SPropertyType::Vector4;
			else if( str == "std::string" ) return SPropertyType::String;
			else if( str == "AssetID" )     return SPropertyType::Uint64;
			else if( str == "UUID" )        return SPropertyType::Uint64;
			else if( str == "AssetReference" ) return SPropertyType::Asset;
			else if( IsValidPointer( str ) ) return SPropertyType::Class;
			else if( "Ref<Entity>" ) return SPropertyType::Entity;
			else /*if( str == "Saturn::SPropertyType::Unknown" )*/ return SPropertyType::Unknown;
		}
		else
		{
			if( str == "char" )                 return SPropertyType::Char;
			else if( str == "float" )           return SPropertyType::Float;
			else if( str == "int" )             return SPropertyType::Int;
			else if( str == "double" )          return SPropertyType::Double;
			else if( str == "uint8_t" )         return SPropertyType::Uint8;
			else if( str == "uint16_t" )        return SPropertyType::Uint16;
			else if( str == "uint32_t" )        return SPropertyType::Uint32;
			else if( str == "uint64_t" )        return SPropertyType::Uint64;
			else if( str == "int8_t" )          return SPropertyType::Int8;
			else if( str == "int16_t" )         return SPropertyType::Int16;
			else if( str == "int64_t" )         return SPropertyType::Int64;
			else if( str == "glm::vec2" )       return SPropertyType::Vector2;
			else if( str == "glm::vec3" )       return SPropertyType::Vector3;
			else if( str == "glm::vec4" )       return SPropertyType::Vector4;
			else if( str == "std::string" )     return SPropertyType::String;
			else if( str == "Saturn::AssetID" ) return SPropertyType::Uint64;
			else if( str == "Saturn::UUID" )    return SPropertyType::Uint64;
			else if( str == "Saturn::AssetReference" ) return SPropertyType::Asset;
			else if( IsValidPointer( str ) )    return SPropertyType::Class;
			else if( "Saturn::Ref<Saturn::Entity>" ) return SPropertyType::Entity;
			else /*if( str == "Saturn::SPropertyType::Unknown" )*/ return SPropertyType::Unknown;
		}
	}

	static std::string SPropertyTypeToString( SPropertyType type )
	{
		switch( type )
		{
			case SPropertyType::Char: return "Saturn::SPropertyType::Char";
			case SPropertyType::Float: return "Saturn::SPropertyType::Float";
			case SPropertyType::Int: return "Saturn::SPropertyType::Int";
			case SPropertyType::Double: return "Saturn::SPropertyType::Double";
			case SPropertyType::Uint8: return "Saturn::SPropertyType::Uint8";
			case SPropertyType::Uint16: return "Saturn::SPropertyType::Uint16";
			case SPropertyType::Uint32: return "Saturn::SPropertyType::Uint32";
			case SPropertyType::Uint64: return "Saturn::SPropertyType::Uint64";
			case SPropertyType::Int8: return "Saturn::SPropertyType::Int8";
			case SPropertyType::Int16: return "Saturn::SPropertyType::Int16";
			case SPropertyType::Int64: return "Saturn::SPropertyType::Int64";
			case SPropertyType::Vector2: return "Saturn::SPropertyType::Vector2";
			case SPropertyType::Vector3: return "Saturn::SPropertyType::Vector3";
			case SPropertyType::Vector4: return "Saturn::SPropertyType::Vector4";
			case SPropertyType::String: return "Saturn::SPropertyType::String";
			case SPropertyType::Asset: return "Saturn::SPropertyType::Asset";
			case SPropertyType::Entity: return "Saturn::SPropertyType::Entity";
			case SPropertyType::Class: return "Saturn::SPropertyType::Class";
			case SPropertyType::Unknown: return "Saturn::SPropertyType::Unknown";

			default: break;
		}

		return "";
	}

	static void CreateGetSetForClassProp( const SProperty& rProperty, const std::string& rClassName, std::ofstream& fout )
	{
		// Set property function
		fout << std::format( "\tstatic void Set{0}( {1}* pClass, {2} pValue )\n", rProperty.GetName(), rClassName, rProperty.GetNativeType() );
		fout << "\t{\n";
		fout << "\t\tpClass->" << rProperty.GetName() << " = pValue;\n";
		fout << "\t}\n";

		// Get property function
		fout << std::format( "\tstatic {0} Get{1}( {2}* pClass )\n", rProperty.GetNativeType(), rProperty.GetName(), rClassName );
		fout << "\t{\n";
		fout << "\t\treturn pClass->" << rProperty.GetName() << ";\n";
		fout << "\t}\n";

		fout << "\n";
	}

	static void CreateGetSetForAssetProp( const SProperty& rProperty, const std::string& rClassName, std::ofstream& fout )
	{
		// Set property function
		fout << std::format( "\tstatic void Set{0}( {1}* pClass, Saturn::AssetID _ASSETID_ )\n", rProperty.GetName(), rClassName );
		fout << "\t{\n";
		fout << "\t\tpClass->" << rProperty.GetName() << ".ID = _ASSETID_;\n";
		fout << "\t}\n";

		// Get property function
		fout << std::format( "\tstatic Saturn::AssetReference& Get{0}( {1}* pClass )\n", rProperty.GetName(), rClassName );
		fout << "\t{\n";
		fout << "\t\treturn pClass->" << rProperty.GetName() << ";\n";
		fout << "\t}\n";

		fout << "\n";
	}

	bool HeaderTool::GenerateHeader( HeaderToolCommand& rCommand ) 
	{
		bool result = true;
		
		std::filesystem::path outputPath = m_WorkingDir;
		outputPath /= rCommand.Filepath.stem();
		outputPath += ".Gen.h";

		std::ofstream fout( outputPath );

		fout << "/* Generated code, DO NOT modify! */\n";
		fout << "#pragma once\n";
		fout << "#include \"Saturn/GameFramework/Core/GameScript.h\"\n\n";

		// Read the header file
		std::ifstream headerFile( rCommand.Filepath );

		bool LastLineHadSP = false, SClassFound = false, GeneratedBodyFound = false, UsingSaturnNamespace = false;

		std::string line;
		int lineNumber = 0;
		while( std::getline( headerFile, line ) )
		{
			lineNumber++;
			if( line.empty() ) continue;

			if( rCommand.ClassName.empty() || rCommand.BaseClass.empty() )
			{
				auto pair = GetClassNameAndBaseClass( line );

				rCommand.ClassName = pair.first;
				rCommand.BaseClass = pair.second;
			}

			if( line.contains( "using namespace Saturn;" ) && LineIsNotComment( line ) )
			{
				UsingSaturnNamespace = true;
			}
			
			std::smatch match;
			
			if( LastLineHadSP && LineIsNotComment( line ) )
			{
				std::regex typeRegex( R"(([\w:]+(?:<[^<>]+>)?(?:\s*[*&]+)*)\s+(\w+))" );

				if( std::regex_search( line, match, typeRegex ) )
				{
					const std::string type = match[ 1 ].str();
					const std::string name = match[ 2 ].str();

					SPropertyType realType = StringToSPropertyType( type, UsingSaturnNamespace );

					SProperty p{ name, type, realType };
					rCommand.Properties[ lineNumber - 1 ] = p;
				}
				else
				{
					std::cout << rCommand.Filepath.string() << s_ErrorsMaps[ HeaderToolError::CG003 ] << "\n";
				}

				LastLineHadSP = false;
			}

			// Check for SPROPERTY
			std::regex spropertyRegex( R"(SPROPERTY\((.*)\))", std::regex::extended );

			if( std::regex_search( line, match, spropertyRegex ) && LineIsNotComment( line ) )
			{
				const std::string args = match[ 1 ].str();
				const std::string remainingContent = line.substr( match.position() + match.length() ).c_str();

				// If SPROPERTY is typed like: SPROPERTY() float Speed;
				// This is handled in the else statement
				// However, if not then we need to parse the next line for the variable definition.
				if( remainingContent.empty() || remainingContent[ 0 ] == '\n' )
				{
					LastLineHadSP = true;
				}
				else
				{
					if( std::regex_search( line, match, std::regex( R"(SPROPERTY\(.*?\)\s+(\w+)\s+(\w+))", std::regex::extended ) ) )
					{
						const std::string type = match[ 1 ].str();
						const std::string name = match[ 2 ].str();

						SPropertyType spropType = StringToSPropertyType( type, UsingSaturnNamespace );

						SProperty p{ name, type, spropType };
						rCommand.Properties[ lineNumber ] = p;
					}
					else
					{
						// Expected variable definition after SPROPERTY macro.
						std::cout << rCommand.Filepath.string() << s_ErrorsMaps[ HeaderToolError::CG003 ] << "\n";
					}
				}


				// SPROPERTY(Asset)
				// [AssetID, UUID, uint64_t, unsigned long long] m_Asset;
				// OR
				// [Saturn::AssetID, Saturn::UUID, uint64_t, unsigned long long] m_Asset;

				// Parse SProperty args
				if( !args.empty() )
				{
					if( args.contains( "Entity" ) )
					{
						rCommand.Properties[ lineNumber ].SetFlag( SPropertyFlags_Entity, true );
						rCommand.Properties[ lineNumber ].SetType( SPropertyType::Entity );
					}
				}
			}

			if( !SClassFound )
			{
				if( std::regex_search( line, match, std::regex( R"(SCLASS\((.*)\))", std::regex::extended ) ) && LineIsNotComment( line ) )
				{
					const std::string args = match[ 1 ].str();

					if( args.contains( "Spawnable" ) )
					{
						rCommand.ClassFlags |= ( uint32_t ) SClassFlags::Spawnable;
					}

					if( args.contains( "VisibleInEditor" ) )
					{
						rCommand.ClassFlags |= ( uint32_t ) SClassFlags::VisibleInEditor;
					}

					if( args.contains( "NoMetadata" ) )
					{
						rCommand.ClassFlags |= ( uint32_t ) SClassFlags::NoMetadata;
					}

					SClassFound = true;
				}
			}

			if( !GeneratedBodyFound )
			{
				if( std::regex_search( line, match, std::regex( R"(GENERATED_BODY\((.*)\))", std::regex::extended ) ) && LineIsNotComment( line ) )
				{
					const std::string args = match[ 1 ].str();

					if( !args.empty() )
					{
						std::cout << rCommand.Filepath.string() << s_WarningMaps[ HeaderToolWarning::CG002A ] << "\n";
					}

					// Parse generated header

					std::string baseFileId = std::format( "FID_{0}_h_{1}", rCommand.ClassName, lineNumber );

					std::string CFI = std::format( "#undef CURRENT_FILE_ID\n#define CURRENT_FILE_ID FID_{0}_h\n\n", rCommand.ClassName );

					fout << CFI;

					std::string idGeneratedBody = std::format( "#define {0}_GENERATED_BODY {0}_CLASSDECLS\n", baseFileId );
					std::string classDecls = std::format( "#define {0}_CLASSDECLS \\\nprivate: \\\n\tSAT_DECLARE_CLASS({1},{2}) \\\npublic:\\\n\n", baseFileId, rCommand.ClassName, rCommand.BaseClass );

					fout << classDecls;
					fout << idGeneratedBody;

					GeneratedBodyFound = true;
				}
			}
		}

		headerFile.close();
		fout.close();

		if( !GeneratedBodyFound )
		{
			std::cout << rCommand.Filepath.string() << s_ErrorsMaps[ HeaderToolError::CG002 ] << "\n";
			result = false;
		}

		if( !SClassFound )
		{
			std::cout << rCommand.Filepath.string() << s_ErrorsMaps[ HeaderToolError::CG001 ] << "\n";
			return false;
		}

		return result;
	}

	bool HeaderTool::GenerateSource( HeaderToolCommand& rCommand )
	{
		bool result = true;

		std::filesystem::path outputPath = m_WorkingDir;
		outputPath /= rCommand.Filepath.stem();
		outputPath += ".Gen.cpp";

		std::filesystem::path generatedHeaderPath = m_WorkingDir;
		generatedHeaderPath /= rCommand.Filepath.stem();
		generatedHeaderPath += ".Gen.h";

		std::ofstream fout( outputPath );

		fout << "/* Generated code, DO NOT modify! */\n";
		fout << "#include \"Saturn/GameFramework/Core/GameScript.h\"\n";
		fout << "#include \"Saturn/GameFramework/Core/ClassMetadataHandler.h\"\n";
		fout << "#include \"Saturn/Scene/Entity.h\"\n";
		fout << std::format( "#include \"{0}\"\n", rCommand.Filepath.string() );
		fout << std::format( "#include \"{0}\"\n\n", generatedHeaderPath.string() );

		auto& rClassName = rCommand.ClassName;

		fout << "extern \"C\" {\n";

		if( ( rCommand.ClassFlags & ( uint32_t ) SClassFlags::Spawnable ) != 0 )
		{
			fout << std::format( "__declspec(dllexport) Saturn::Entity* _Z_Create_{0}(Saturn::Scene* pScene)\n", rClassName );
			fout << "{\n";
			fout << std::format( "\tSaturn::Ref<{0}> Target = Saturn::Ref<{0}>::Create();\n", rClassName );
			fout << "\tSaturn::Ref<Saturn::Entity> TargetReturn = Target.As<Saturn::Entity>();\n";
			fout << "\treturn TargetReturn.Get();\n";
			fout << "}\n";

			fout << "//^^^ Spawnable\n";
		}
		else
		{
			fout << std::format( "__declspec(dllexport) Saturn::Entity* _Z_Create_{0}(Saturn::Scene* pScene)\n", rClassName );
			fout << "{\n";
			fout << "\treturn nullptr;\n";
			fout << "}\n";

			fout << "//^^^ NO Spawnable\n";
		}

		fout << "}\n\n";

		if( ( rCommand.ClassFlags & ( uint32_t ) SClassFlags::NoMetadata ) == 0 )
		{
			std::string realPath = rCommand.Filepath.string();

#if defined(SAT_PLATFORM_WINDOWS)
			size_t pos = 0;
			while( ( pos = realPath.find( '\\', pos ) ) != std::string::npos )
			{
				realPath.replace( pos, 1, "\\\\" );
				pos += 2;
			}
#endif

			fout << std::format( "static void ReflCreateMetadataFor_{0}()\n", rClassName );
			fout << "{\n";
			fout << std::format( "\tSaturn::SClassMetadata __Metadata_{0};\n", rClassName );
			fout << std::format( "\t__Metadata_{0}.Name = \"{0}\";\n", rClassName );
			fout << std::format( "\t__Metadata_{0}.ParentClassName = \"{1}\";\n", rClassName, rCommand.BaseClass );
			fout << std::format( "\t__Metadata_{0}.GeneratedSourcePath = __FILE__;\n", rClassName );
			fout << std::format( "\t__Metadata_{0}.HeaderPath = \"{1}\";\n", rClassName, realPath );
			fout << std::format( "\t__Metadata_{0}.ExternalData = true;\n", rClassName );
			fout << std::format( "\tSaturn::ClassMetadataHandler::Get().AddMetadata( __Metadata_{0} );\n", rClassName );
			fout << "}\n\n";
		}
		else
		{
			fout << std::format( "static void ReflCreateMetadataFor_{0}()\n", rClassName );
			fout << "{\n";
			fout << std::format( "\tSaturn::SClassMetadata __Metadata_{0};\n", rClassName );
			fout << std::format( "\t__Metadata_{0}.Name = \"{0}\";\n", rClassName );
			fout << std::format( "\tSaturn::ClassMetadataHandler::Get().AddMetadata( __Metadata_{0} );\n", rClassName );
			fout << "}\n\n";
		}

		std::string internalClassName = std::format( "{0}Int", rClassName );

		fout << "class " << internalClassName << "\n";
		fout << "{\n";
		fout << "public:\n";

		// Properties -- getters and setters for internal class
		for( const auto& [lineNumber, rProperty] : rCommand.Properties )
		{
			if( rProperty.GetType() == SPropertyType::Class ) 
			{
				CreateGetSetForClassProp( rProperty, rClassName, fout );
				continue;
			}
			else if( rProperty.GetType() == SPropertyType::Asset ) 
			{
				CreateGetSetForAssetProp( rProperty, rClassName, fout );
				continue;
			}

			std::string stringType = SPropertyTypeToString( rProperty.GetType() );

			// Set property function
			fout << "\tstatic void Set" << rProperty.GetName() << "( " << rClassName << "* pClass, " << "typename Saturn::PropertyTypeTraits< " << stringType << ">::Type" << " value )\n";
			fout << "\t{\n";
			fout << "\t\tpClass->" << rProperty.GetName() << " = value;\n";
			fout << "\t}\n";

			// Get property function
			fout << "\tstatic typename Saturn::PropertyTypeTraits<" << stringType << ">::Type" << " Get" << rProperty.GetName() << "( " << rClassName << "* pClass )\n";
			fout << "\t{\n";
			fout << "\t\treturn pClass->" << rProperty.GetName() << ";\n";
			fout << "\t}\n";

			fout << "\n";
		}

		fout << "};\n\n";

		fout << "static void ReflRegisterPropertiesFor_" << rClassName << "()\n";
		fout << "{\n";

		for( const auto& [lineNumber, rProperty] : rCommand.Properties )
		{
			std::string stringType = SPropertyTypeToString( rProperty.GetType() );

			fout << std::format( "\tSaturn::SProperty Prop_{0};\n", rProperty.GetName() );
			
			fout << std::format( "\tProp_{0}.SetName( \"{0}\" );\n", rProperty.GetName() );
			fout << std::format( "\tProp_{0}.SetType( {1} );\n", rProperty.GetName(), stringType );

			fout << std::format( "\tProp_{0}.pGetPropertyFunction = &{1}::Get{0};\n", rProperty.GetName(), internalClassName );
			fout << std::format( "\tProp_{0}.pSetPropertyFunction = &{1}::Set{0};\n", rProperty.GetName(), internalClassName );

			fout << std::format( "\tSaturn::ClassMetadataHandler::Get().RegisterProperty( \"{0}\", Prop_{1} );\n", rClassName, rProperty.GetName() );
		}

		fout << "}\n\n";

		// Auto-Registration (DLL only).
		fout << std::format( "struct Ar{0}_RTEditor\n", rClassName );
		fout << "{\n";
		fout << std::format( "\tAr{0}_RTEditor()\n", rClassName );
		fout << "\t{\n";
		fout << std::format( "\t\tReflCreateMetadataFor_{0}();\n", rClassName );
		fout << std::format( "\t\tReflRegisterPropertiesFor_{0}();\n", rClassName );
		fout << "\t}\n";
		fout << "};\n\n";
		fout << std::format( "static Ar{0}_RTEditor Ar{0}_Runtime;\n", rClassName );
		fout << "//^^^ Auto-Registration\n";

		fout.close();

		return result;
	}
}