using System;

namespace SaturnBuildTool
{
    static internal class VulkanSDKPath
    {
        public static string FindVulkanSDKPath() 
        {
            string binPath = Environment.GetEnvironmentVariable( "VULKAN_SDK_PATH" );
            if( binPath == null )
            {
                binPath = Environment.GetEnvironmentVariable( "VK_SDK_PATH" );
                if( binPath == null )
                {
                    binPath = Environment.GetEnvironmentVariable( "VULKAN_SDK" );

                    if( string.IsNullOrEmpty( binPath ) )
                    {
                        Console.WriteLine( "ERORR: Unable to find Vulkan SDK! Used environment variable: VULKAN_SDK_PATH, VK_SDK_PATH and VULKAN_SDK!" );

                        return "";
                    }
                }
            }

            return binPath;
        }
    }
}
