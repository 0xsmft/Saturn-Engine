/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2026 BEAST                                                           *
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

#pragma once

namespace Saturn::Auxiliary {

	constexpr std::string CountryToString( Country country )
	{
		switch( country )
		{
			case Country::Unknown: return "Unknown";
			case Country::Afghanistan: return "Afghanistan";
			case Country::Albania: return "Albania";
			case Country::Algeria: return "Algeria";
			case Country::Andorra: return "Andorra";
			case Country::Angola: return "Angola";
			case Country::AntiguaAndBarbuda: return "Antigua and Barbuda";
			case Country::Argentina: return "Argentina";
			case Country::Armenia: return "Armenia";
			case Country::Australia: return "Australia";
			case Country::Austria: return "Austria";
			case Country::Azerbaijan: return "Azerbaijan";
			case Country::Bahamas: return "Bahamas";
			case Country::Bahrain: return "Bahrain";
			case Country::Bangladesh: return "Bangladesh";
			case Country::Barbados: return "Barbados";
			case Country::Belarus: return "Belarus";
			case Country::Belgium: return "Belgium";
			case Country::Belize: return "Belize";
			case Country::Benin: return "Benin";
			case Country::Bhutan: return "Bhutan";
			case Country::Bolivia: return "Bolivia";
			case Country::BosniaAndHerzegovina: return "Bosnia and Herzegovina";
			case Country::Botswana: return "Botswana";
			case Country::Brazil: return "Brazil";
			case Country::Brunei: return "Brunei";
			case Country::Bulgaria: return "Bulgaria";
			case Country::BurkinaFaso: return "Burkina Faso";
			case Country::Burundi: return "Burundi";
			case Country::CaboVerde: return "Cabo Verde";
			case Country::Cambodia: return "Cambodia";
			case Country::Cameroon: return "Cameroon";
			case Country::Canada: return "Canada";
			case Country::CentralAfricanRepublic: return "Central African Republic";
			case Country::Chad: return "Chad";
			case Country::Chile: return "Chile";
			case Country::China: return "China";
			case Country::Colombia: return "Colombia";
			case Country::Comoros: return "Comoros";
			case Country::CostaRica: return "Costa Rica";
			case Country::Croatia: return "Croatia";
			case Country::Cuba: return "Cuba";
			case Country::Cyprus: return "Cyprus";
			case Country::Czechia: return "Czechia";
			case Country::DemocraticRepublicOfTheCongo: return "Congo, The Democratic Republic of";
			case Country::Denmark: return "Denmark";
			case Country::Djibouti: return "Djibouti";
			case Country::Dominica: return "Dominica";
			case Country::DominicanRepublic: return "Dominican Republic";
			case Country::Ecuador: return "Ecuador";
			case Country::Egypt: return "Egypt";
			case Country::ElSalvador: return "El Salvador";
			case Country::EquatorialGuinea: return "Equatorial Guinea";
			case Country::Eritrea: return "Eritrea";
			case Country::Estonia: return "Estonia";
			case Country::Eswatini: return "Eswatini";
			case Country::Ethiopia: return "Ethiopia";
			case Country::Fiji: return "Fiji";
			case Country::Finland: return "Finland";
			case Country::France: return "France";
			case Country::Gabon: return "Gabon";
			case Country::Gambia: return "Gambia";
			case Country::Georgia: return "Georgia";
			case Country::Germany: return "Germany";
			case Country::Ghana: return "Ghana";
			case Country::Greece: return "Greece";
			case Country::Grenada: return "Grenada";
			case Country::Guatemala: return "Guatemala";
			case Country::Guinea: return "Guinea";
			case Country::GuineaBissau: return "Guinea Bissau";
			case Country::Guyana: return "Guyana";
			case Country::Haiti: return "Haiti";
			case Country::Honduras: return "Honduras";
			case Country::Hungary: return "Hungary";
			case Country::Iceland: return "Iceland";
			case Country::India: return "India";
			case Country::Indonesia: return "Indonesia";
			case Country::Iran: return "Iran";
			case Country::Iraq: return "Iraq";
			case Country::Ireland: return "Ireland";
			case Country::Israel: return "Israel";
			case Country::Italy: return "Italy";
			case Country::Jamaica: return "Jamaica";
			case Country::Japan: return "Japan";
			case Country::Jordan: return "Jordan";
			case Country::Kazakhstan: return "Kazakhstan";
			case Country::Kenya: return "Kenya";
			case Country::Kiribati: return "Kiribati";
			case Country::Kuwait: return "Kuwait";
			case Country::Kyrgyzstan: return "Kyrgyzstan";
			case Country::Laos: return "Laos";
			case Country::Latvia: return "Latvia";
			case Country::Lebanon: return "Lebanon";
			case Country::Lesotho: return "Lesotho";
			case Country::Liberia: return "Liberia";
			case Country::Libya: return "Libya";
			case Country::Liechtenstein: return "Liechtenstein";
			case Country::Lithuania: return "Lithuania";
			case Country::Luxembourg: return "Luxembourg";
			case Country::Madagascar: return "Madagascar";
			case Country::Malawi: return "Malawi";
			case Country::Malaysia: return "Malaysia";
			case Country::Maldives: return "Maldives";
			case Country::Mali: return "Mali";
			case Country::Malta: return "Malta";
			case Country::MarshallIslands: return "Marshall Islands";
			case Country::Mauritania: return "Mauritania";
			case Country::Mauritius: return "Mauritius";
			case Country::Mexico: return "Mexico";
			case Country::Micronesia: return "Micronesia";
			case Country::Moldova: return "Moldova";
			case Country::Monaco: return "Monaco";
			case Country::Mongolia: return "Mongolia";
			case Country::Montenegro: return "Montenegro";
			case Country::Morocco: return "Morocco";
			case Country::Mozambique: return "Mozambique";
			case Country::Myanmar: return "Myanmar";
			case Country::Namibia: return "Namibia";
			case Country::Nauru: return "Nauru";
			case Country::Nepal: return "Nepal";
			case Country::Netherlands: return "Netherlands";
			case Country::NewZealand: return "New Zealand";
			case Country::Nicaragua: return "Nicaragua";
			case Country::Niger: return "Niger";
			case Country::Nigeria: return "Nigeria";
			case Country::NorthKorea: return "Korea, Democratic People's Republic";
			case Country::NorthMacedonia: return "North Macedonia";
			case Country::Norway: return "Norway";
			case Country::Oman: return "Oman";
			case Country::Pakistan: return "Pakistan";
			case Country::Palau: return "Palau";
			case Country::Palestine: return "Palestine";
			case Country::Panama: return "Panama";
			case Country::PapuaNewGuinea: return "Papua New Guinea";
			case Country::Paraguay: return "Paraguay";
			case Country::Peru: return "Peru";
			case Country::Philippines: return "Philippines";
			case Country::Poland: return "Poland";
			case Country::Portugal: return "Portugal";
			case Country::Qatar: return "Qatar";
			case Country::RepublicOfTheCongo: return "Congo, The Republic of";
			case Country::Romania: return "Romania";
			case Country::Russia: return "Russia";
			case Country::Rwanda: return "Rwanda";
			case Country::SaintKittsAndNevis: return "Saint Kitts and Nevis";
			case Country::SaintLucia: return "SaintLucia";
			case Country::SaintVincentAndTheGrenadines: return "Saint Vincent and The Grenadines";
			case Country::Samoa: return "Samoa";
			case Country::SanMarino: return "San Marino";
			case Country::SaoTomeAndPrincipe: return "Sao Tome and Principe";
			case Country::SaudiArabia: return "Saudi Arabia";
			case Country::Senegal: return "Senegal";
			case Country::Serbia: return "Serbia";
			case Country::Seychelles: return "Seychelles";
			case Country::SierraLeone: return "Sierra Leone";
			case Country::Singapore: return "Singapore";
			case Country::Slovakia: return "Slovakia";
			case Country::Slovenia: return "Slovenia";
			case Country::SolomonIslands: return "Solomon Islands";
			case Country::Somalia: return "Somalia";
			case Country::SouthAfrica: return "South Africa";
			case Country::SouthKorea: return "Korea, Republic of";
			case Country::SouthSudan: return "South Sudan";
			case Country::Spain: return "Spain";
			case Country::SriLanka: return "Sri Lanka";
			case Country::Sudan: return "Sudan";
			case Country::Suriname: return "Suriname";
			case Country::Sweden: return "Sweden";
			case Country::Switzerland: return "Switzerland";
			case Country::Syria: return "Syria";
			case Country::Tajikistan: return "Tajikistan";
			case Country::Tanzania: return "Tanzania";
			case Country::Thailand: return "Thailand";
			case Country::TimorLeste: return "Timor Leste";
			case Country::Togo: return "Togo";
			case Country::Tonga: return "Tonga";
			case Country::TrinidadAndTobago: return "Trinidad and Tobago";
			case Country::Tunisia: return "Tunisia";
			case Country::Turkiye: return "Turkiye";
			case Country::Turkmenistan: return "Turkmenistan";
			case Country::Tuvalu: return "Tuvalu";
			case Country::Uganda: return "Uganda";
			case Country::Ukraine: return "Ukraine";
			case Country::UnitedArabEmirates: return "United Arab Emirates";
			case Country::UnitedKingdom: return "United Kingdom";
			case Country::UnitedStates: return "United States";
			case Country::Uruguay: return "Uruguay";
			case Country::Uzbekistan: return "Uzbekistan";
			case Country::Vanuatu: return "Vanuatu";
			case Country::VaticanCity: return "VaticanCity";
			case Country::Venezuela: return "Venezuela";
			case Country::Vietnam: return "Vietnam";
			case Country::Yemen: return "Yemen";
			case Country::Zambia: return "Zambia";
			case Country::Zimbabwe: return "Zimbabwe";
			default: return "Unknown";
		}
	}
	
}
