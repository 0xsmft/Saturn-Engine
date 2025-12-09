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
#include <execution>
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

	void HeaderTool::SubmitWorkList( const std::vector<std::filesystem::path>& rCommands, HeaderToolConfigKind config )
	{
		m_Commands.reserve( rCommands.size() );

		for( const auto& rCommand : rCommands )
		{
			m_Commands.emplace_back( rCommand, config );
		}
	}

	bool HeaderTool::StartGeneration()
	{
#if defined(SAT_HEADERTOOL_MT)
		std::atomic_uint tasksFailed = 0;

		std::for_each( std::execution::par, m_Commands.begin(), m_Commands.end(), 
			[ & ](auto& rCommand)
		{
			const auto result = GenerateHeader( rCommand );

			switch( result )
			{
				case HeaderToolParseResult::Success:
				{
					if( !GenerateSource( rCommand ) )
						tasksFailed++;
				} break;

				case HeaderToolParseResult::ParseSkipped:
					break;

					// Anything else, count as an error.
				default:
					tasksFailed++;
					break;
			}
		} );

		return tasksFailed.load() == 0;
#else
		uint32_t tasksFailed = 0;

		for( auto& rCommand : m_Commands )
		{
			const auto result = GenerateHeader( rCommand );

			switch( result )
			{
				case HeaderToolParseResult::Success:
				{
					if( !GenerateSource( rCommand ) ) 
						++tasksFailed;
				} break;

				case HeaderToolParseResult::ParseSkipped:
					break;

				// Anything else, count as an error.
				default:
					++tasksFailed;
					break;
			}
		}

		return tasksFailed == 0;
#endif
	}

	[[nodiscard]] static bool LineIsComment( const std::string& rLine )
	{
		std::regex regex( R"(^\s*(//.*|/\*.*\*/|/\*.*))" );
		return std::regex_match( rLine, regex );
	}

	[[nodiscard]] static bool LineIsPreprocessor( const std::string& rLine )
	{
		std::regex regex( R"(^\s*#)" );
		return std::regex_search( rLine, regex );
	}

	static std::pair<std::string, std::string> GetClassNameAndBaseClass( const std::string& rLine )
	{
		std::regex classPattern( R"(class\s+(\w+)\s*:\s*public\s+(\w+))" );
//		std::regex fullClassPattern( R"(\bclass\s+(\w+)(\s*:\s*public\s+(\w+))?\s*\{)" );
		std::smatch match;

		// NOTE: Returns the first base class
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
			else if( "Ref<Entity>" ) return SPropertyType::EntityType;
			else /*if( str == "Saturn::SPropertyType::Unknown" )*/ return SPropertyType::Unknown;
		}
		else
		{
			if( str == "char" )							return SPropertyType::Char;
			else if( str == "float" )					return SPropertyType::Float;
			else if( str == "int" )						return SPropertyType::Int;
			else if( str == "double" )					return SPropertyType::Double;
			else if( str == "uint8_t" )					return SPropertyType::Uint8;
			else if( str == "uint16_t" )				return SPropertyType::Uint16;
			else if( str == "uint32_t" )				return SPropertyType::Uint32;
			else if( str == "uint64_t" )				return SPropertyType::Uint64;
			else if( str == "int8_t" )					return SPropertyType::Int8;
			else if( str == "int16_t" )					return SPropertyType::Int16;
			else if( str == "int64_t" )					return SPropertyType::Int64;
			else if( str == "glm::vec2" )				return SPropertyType::Vector2;
			else if( str == "glm::vec3" )				return SPropertyType::Vector3;
			else if( str == "glm::vec4" )				return SPropertyType::Vector4;
			else if( str == "std::string" )				return SPropertyType::String;
			else if( str == "Saturn::AssetID" )			return SPropertyType::Uint64;
			else if( str == "Saturn::UUID" )			return SPropertyType::Uint64;
			else if( str == "Saturn::AssetReference" )  return SPropertyType::Asset;
			else if( IsValidPointer( str ) )            return SPropertyType::Class;
			else if( "Saturn::Ref<Saturn::Entity>" )    return SPropertyType::EntityType;
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
			case SPropertyType::EntityType: return "Saturn::SPropertyType::Entity";
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
		fout << std::format( "\tstatic const {0} Get{1}( const {2}* pClass )\n", rProperty.GetNativeType(), rProperty.GetName(), rClassName );
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
		fout << std::format( "\tstatic const Saturn::AssetReference& Get{0}( const {1}* pClass )\n", rProperty.GetName(), rClassName );
		fout << "\t{\n";
		fout << "\t\treturn pClass->" << rProperty.GetName() << ";\n";
		fout << "\t}\n";

		fout << "\n";
	}

	static void WriteGeneratedBody( std::ofstream& fout, HeaderToolCommand& rCommand )
	{
		// Parse generated header
		fout << std::format( "#undef CURRENT_FILE_ID\n#define CURRENT_FILE_ID FID_{0}_h\n\n", rCommand.ClassName );

		const std::string baseFileId = std::format( "FID_{0}_h_{1}", rCommand.ClassName, rCommand.LineNumberForGeneratedBody );

		// Could result in #define FID_MyClass_24_GENERATED_BODY FID_MyClass_24_CLASSDECLS
		const std::string idGeneratedBody = std::format( "#define {0}_GENERATED_BODY {0}_CLASSDECLS\n", baseFileId );

		const std::string classDecls = std::format( "#define {0}_CLASSDECLS \\\nprivate: \\\n\tSAT_DECLARE_CLASS({1},{2}) \\\npublic:\\\n\n", baseFileId, rCommand.ClassName, rCommand.BaseClass );

		fout << classDecls;
		fout << idGeneratedBody;
	}

	static void WriteSClassSpecification( std::ofstream& fout, HeaderToolCommand& rCommand, const std::string& rPropPointersName )
	{
		const uint64_t classHash = FNV1A64( rCommand.ClassName.c_str() );

		const int propSize = ( int ) rCommand.Properties.size();

#if defined(SAT_PLATFORM_WINDOWS)
		std::string realPath = rCommand.Filepath.string();

		if( ( rCommand.ClassFlags & SC_NoExtendedMetadata ) == 0 )
		{
			// Convert the path from C:\MyPath to C:\\MyPath
			// Because it we don't it will output it as the string literal
			size_t pos = 0;
			while( ( pos = realPath.find( '\\', pos ) ) != std::string::npos )
			{
				realPath.replace( pos, 1, "\\\\" );
				pos += 2;
			}
		}
#else
		const std::string realPath = rCommand.Filepath.string();
#endif

		switch( rCommand.ConfigKind )
		{
			default:
			case HeaderToolConfigKind::Unknown:
			case HeaderToolConfigKind::Debug:
			case HeaderToolConfigKind::Release: 
			{
				fout << std::vformat( "\t\tconst SClassSpecification spec{{ \"{0}\", ( SClassFlags ) {1}, {2}, sizeof( {0} ), alignof( {0} ), {5}llu, {0}::Super::StaticClass(), Saturn::RInternalConstructor<{0}>, RStaticLnk{0}, {4}, Saturn::SClassExtendedMetadata{{ \"{3}\" }} }};\n",
					std::make_format_args( rCommand.ClassName, rCommand.ClassFlags, propSize, realPath, rPropPointersName, classHash ) );
			} break;

			case HeaderToolConfigKind::Dist:
			{
				fout << std::vformat( "\t\tconst SClassSpecification spec{{ \"{0}\", ( SClassFlags ) {1}, {2}, sizeof( {0} ), alignof( {0} ), {4}llu, {0}::Super::StaticClass(), Saturn::RInternalConstructor<{0}>, RStaticLnk{0}, {3}, Saturn::SClassExtendedMetadata{{ }} }};\n",
					std::make_format_args( rCommand.ClassName, rCommand.ClassFlags, propSize, rPropPointersName, classHash ) );
			} break;
		}
	}

	HeaderToolParseResult HeaderTool::ParseHeaderFile( HeaderToolCommand& rCommand )
	{
		HeaderToolParseResult result = HeaderToolParseResult::Success;

		// Read the header file
		std::ifstream headerFile( rCommand.Filepath );

		bool LastLineHadSP = false, SClassFound = false, GeneratedBodyFound = false, UsingSaturnNamespace = false;

		std::string line;
		uint32_t lineNumber = 0;
		while( std::getline( headerFile, line ) )
		{
			lineNumber++;
			if( line.empty() ) continue;
			if( LineIsComment( line ) ) continue;
			if( LineIsPreprocessor( line ) ) continue;

			if( rCommand.ClassName.empty() || rCommand.BaseClass.empty() )
			{
				const auto [className, baseClass] = GetClassNameAndBaseClass( line );

				rCommand.ClassName = className;
				rCommand.BaseClass = baseClass;
			}

			if( line.contains( "using namespace Saturn;" ) )
			{
				// Use qualified names
				UsingSaturnNamespace = true;
			}

			std::smatch match;

			if( LastLineHadSP )
			{
				std::regex typeRegex( R"(([\w:]+(?:<[^<>]+>)?(?:\s*[*&]+)*)\s+(\w+))" );

				if( std::regex_search( line, match, typeRegex ) )
				{
					const std::string type = match[ 1 ].str();
					const std::string name = match[ 2 ].str();

					const SPropertyType realType = StringToSPropertyType( type, UsingSaturnNamespace );

					rCommand.Properties.emplace(
						std::piecewise_construct,
						std::forward_as_tuple( lineNumber - 1 ),
						std::forward_as_tuple( name, type, realType ) );
				}
				else
				{
					std::cout << rCommand.Filepath.string() << s_ErrorsMaps[ HeaderToolError::CG003 ] << "\n";
					result = HeaderToolParseResult::FailedToParse;
				}

				LastLineHadSP = false;
			}

			// Check for SPROPERTY
			std::regex spropertyRegex( R"(SPROPERTY\((.*)\))", std::regex::extended );

			if( std::regex_search( line, match, spropertyRegex ) )
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

						const SPropertyType spropType = StringToSPropertyType( type, UsingSaturnNamespace );

						rCommand.Properties.emplace(
							std::piecewise_construct,
							std::forward_as_tuple( lineNumber - 1 ),
							std::forward_as_tuple( name, type, spropType ) );
					}
					else
					{
						// Expected variable definition after SPROPERTY macro.
						std::cout << rCommand.Filepath.string() << s_ErrorsMaps[ HeaderToolError::CG003 ] << "\n";
						result = HeaderToolParseResult::FailedToParse;
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
						rCommand.Properties[ lineNumber ].SetFlag( SPropertyFlags_EntityType, true );
						rCommand.Properties[ lineNumber ].SetType( SPropertyType::EntityType );
					}
				}
			}

			if( !SClassFound )
			{
				if( std::regex_search( line, match, std::regex( R"(SCLASS\((.*)\))", std::regex::extended ) ) )
				{
					const std::string args = match[ 1 ].str();

					if( args.contains( "Spawnable" ) )
					{
						rCommand.ClassFlags |= ( uint32_t ) SClassFlags::SC_Spawnable;
					}

					if( args.contains( "VisibleInEditor" ) )
					{
						rCommand.ClassFlags |= ( uint32_t ) SClassFlags::SC_VisibleInEditor;
					}

					if( args.contains( "NoMetadata" ) || args.contains( "NoExtendedMetadata" ) )
					{
						rCommand.ClassFlags |= ( uint32_t ) SClassFlags::SC_NoExtendedMetadata;
					}

					SClassFound = true;
				}
			}

			if( !GeneratedBodyFound )
			{
				if( std::regex_search( line, match, std::regex( R"(GENERATED_BODY\((.*)\))", std::regex::extended ) ) )
				{
					const std::string args = match[ 1 ].str();

					if( !args.empty() )
					{
						std::cout << rCommand.Filepath.string() << s_WarningMaps[ HeaderToolWarning::CG002A ] << "\n";
					}

					rCommand.LineNumberForGeneratedBody = lineNumber;
					GeneratedBodyFound = true;
				}
			}
		}

		headerFile.close();

		if( SClassFound && !GeneratedBodyFound )
		{
			std::cout << rCommand.Filepath.string() << s_ErrorsMaps[ HeaderToolError::CG002 ] << "\n";
			result = HeaderToolParseResult::NoGeneratedBody;

			if( rCommand.BaseClass.empty() )
			{
				std::cout << rCommand.Filepath.string() << s_ErrorsMaps[ HeaderToolError::CG004 ] << "\n";
			}
		}
		else if( !SClassFound && GeneratedBodyFound )
		{
			std::cout << rCommand.Filepath.string() << s_ErrorsMaps[ HeaderToolError::CG001 ] << "\n";
			result = HeaderToolParseResult::NoSClass;
		}
		else if( !SClassFound && !GeneratedBodyFound )
		{
			result = HeaderToolParseResult::ParseSkipped;
		}

		return ( HeaderToolParseResult ) result;
	}

	HeaderToolParseResult HeaderTool::GenerateHeader( HeaderToolCommand& rCommand )
	{
		HeaderToolParseResult result = ParseHeaderFile( rCommand );
		
		if( result == HeaderToolParseResult::Success )
		{
			std::filesystem::path outputPath = m_WorkingDir;
			outputPath /= rCommand.Filepath.stem();
			outputPath += ".Gen.h";

			std::ofstream fout( outputPath );

			fout << "/* Generated code, DO NOT modify! */\n";
			fout << "#pragma once\n";
			fout << "#include \"Saturn/GameFramework/Core/GameScript.h\"\n\n";

			WriteGeneratedBody( fout, rCommand );

			fout.close();
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

		generatedHeaderPath = std::filesystem::relative( generatedHeaderPath, m_WorkingDir );

		std::ofstream fout( outputPath );

		fout << "/* Generated code, DO NOT modify! */\n";
		if( !m_PCHPath.empty() )
		{
			fout << "#include \"sppch.h\"\n";
		}
		fout << "#include \"Saturn/GameFramework/Core/GameScript.h\"\n";
		fout << "#include \"Saturn/GameFramework/SClass.h\"\n";
		fout << "#include \"Saturn/GameFramework/Core/ClassMetadataHandler.h\"\n";
		fout << "#include \"Saturn/Scene/Entity.h\"\n";

		std::filesystem::path sourceDir = m_WorkingDir.parent_path().parent_path();
		sourceDir /= "Source";

		// TODO: On Windows this will result in us using the windows file system separator i.e. "\" instead of using / 
		std::filesystem::path relativePath = std::filesystem::relative( rCommand.Filepath, sourceDir );

		fout << std::format( "#include \"{0}\"\n", rCommand.Filepath.string() );
		fout << std::format( "#include \"{0}\"\n\n", generatedHeaderPath.string() );

		const auto& rClassName = rCommand.ClassName;

		fout << "using namespace Saturn;\n";
		fout << std::format( "void LinkSymbol{0}() {{}}\n", rClassName );
		fout << std::format( "__declspec(dllexport) Saturn::SClass* RStaticLnk{0}();\n\n", rClassName );

		// [BEGIN INTERNAL CLASS]
		const std::string internalClassName = std::format( "{0}Int", rClassName );
		fout << "class " << internalClassName << "\n";
		fout << "{\n";
		fout << "public:\n";

		// Properties
		const bool classHasSProps = !rCommand.Properties.empty();

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

			const std::string stringType = SPropertyTypeToString( rProperty.GetType() );

			// Set property function
			fout << "\tstatic void Set" << rProperty.GetName() << "( " << rClassName << "* pClass, " << "typename Saturn::PropertyTypeTraits< " << stringType << ">::Type" << " value )\n";
			fout << "\t{\n";
			fout << "\t\tpClass->" << rProperty.GetName() << " = value;\n";
			fout << "\t}\n";

			// Get property function
			fout << "\tstatic typename Saturn::PropertyTypeTraits<" << stringType << ">::Type" << " Get" << rProperty.GetName() << "( const " << rClassName << "* pClass )\n";
			fout << "\t{\n";
			fout << "\t\treturn pClass->" << rProperty.GetName() << ";\n";
			fout << "\t}\n";

			fout << "\n";
		}

		fout << "};\n";
		fout << "//^^^ [END INTERNAL CLASS]\n\n";
		// ^^^ [END INTERNAL CLASS]

		if( classHasSProps )
		{
			// Declare SProperty
			for( const auto& [lineNumber, rProperty] : rCommand.Properties )
			{
				const std::string stringType = SPropertyTypeToString( rProperty.GetType() );

				fout << std::format( "const Saturn::SProperty Prop_{0}( \"{0}\", {1}, &{2}::Get{0}, &{2}::Set{0} );\n", rProperty.GetName(), stringType, internalClassName );
			}

			// PropertyPointers array
			fout << "const Saturn::SProperty* const PropertyPointers[] =\n{";
			for( const auto& [lineNumber, rProperty] : rCommand.Properties )
			{
				fout << std::format( "\t(const Saturn::SProperty*)&Prop_{0},\n", rProperty.GetName() );
			}
			fout << "};\n";
		}

		// Class Auto-Registration
		fout << std::format( "Saturn::SClass* RStaticLnk{0}()\n", rClassName );
		fout << "{\n";
		fout << "\tstatic Saturn::SClass* pClass = nullptr;\n";
		fout << "\tif( !pClass )\n";
		fout << "\t{\n";

		const std::string propertyPointersFieldName = classHasSProps ? "PropertyPointers" : "nullptr";
		WriteSClassSpecification( fout, rCommand, propertyPointersFieldName );
	
		fout << "\t\tSClass::RConstructClass( &pClass, spec );\n";
		fout << "\t}\n\n";
		fout << "\treturn pClass;\n";
		fout << "}\n\n";

		fout << std::vformat( 
			"static Saturn::SClassRegistrar RCR{0}( RStaticLnk{0} );\n", std::make_format_args( rClassName ) );
	
		// ^^^ results in static SClassRegistrar RCRMyClass( 
		// SClassSpecification{ 
		// "MyClass", 
		// ( SClassFlags ) Saturn::SClassFlags::SC_None, 
		// sizeof( MyClass ), alignof( MyClass ), 
		// Saturn::RInternalConstructor<MyClass>, 
		// RStaticLnkMyClass 
		// } );

		fout << "//^^^ Auto-Registration\n\n";

		// GetStaticClassInternal
		fout << "//vvv GetStaticClassInternal X31\n";
		fout << std::format( "Saturn::SClass* {0}::GetStaticClassInternal()\n", rCommand.ClassName );
		fout << "{\n";
		fout << std::format( "\treturn RStaticLnk{0}();\n", rCommand.ClassName );
		fout << "}\n";

		fout << "\n// [END OF GENERATED FILE]\n";

		fout.close();

		return result;
	}
}
