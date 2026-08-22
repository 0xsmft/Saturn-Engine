-- Taken from Hazel.....

require('vstudio')
local cs2005 = premake.vstudio.cs2005
local dotnetbase = premake.vstudio.dotnetbase

premake.api.register {
    name = "propertytags",
    scope = "project",
    kind = "list:string",
}

local function insertProperties(prj)
    local elementcount = #prj.propertytags
    if (elementcount % 2) == 0 and elementcount > 0 then
        for i=1, elementcount, 2 do
            local property = prj.propertytags[i]
            local value = prj.propertytags[i + 1]
            _p(2, '<' .. property .. '>' .. value .. '</' .. property .. '>')
        end
    end
end

premake.override(dotnetbase, "projectProperties", function(base, prj)
    _p(1,'<PropertyGroup>')
    local cfg = premake.project.getfirstconfig(prj)
    premake.callArray(dotnetbase.elements.projectProperties, cfg)
    -- Function goes at the end for style reasons... could go into callArray if introduced.
    insertProperties(prj)
    _p(1,'</PropertyGroup>')
end)
