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

	std::string LocTextLanguageToString( LocTextLanguage language )
	{
		switch( language )
		{
			case LocTextLanguage::Unknown: return "Unknown";
			case LocTextLanguage::English: return "English";
			case LocTextLanguage::MandarinChinese: return "Chinese, Mandarin";
			case LocTextLanguage::Spanish: return "Spanish";
			case LocTextLanguage::Hindi: return "Hindi";
			case LocTextLanguage::Arabic: return "Arabic";
			case LocTextLanguage::French: return "French";
			case LocTextLanguage::Bengali: return "Bengali";
			case LocTextLanguage::Portuguese: return "Portuguese";
			case LocTextLanguage::Russian: return "Russian";
			case LocTextLanguage::Urdu: return "Urdu";
			case LocTextLanguage::Indonesian: return "Indonesian";
			case LocTextLanguage::German: return "German";
			case LocTextLanguage::Japanese: return "Japanese";
			case LocTextLanguage::NigerianPidgin: return "Pidgin (Nigerian)";
			case LocTextLanguage::Marathi: return "Marathi";
			case LocTextLanguage::Telugu: return "Telugu";
			case LocTextLanguage::Turkish: return "Turkish";
			case LocTextLanguage::Tamil: return "Tamil";
			case LocTextLanguage::YueChinese: return "Chinese, Yue";
			case LocTextLanguage::WuChinese: return "Chinese, Wu";
			case LocTextLanguage::Korean: return "Korean";
			case LocTextLanguage::Persian: return "Persian";
			case LocTextLanguage::Hausa: return "Hausa";
			case LocTextLanguage::Swahili: return "Swahili";
			case LocTextLanguage::Javanese: return "Javanese";
			case LocTextLanguage::Italian: return "Italian";
			case LocTextLanguage::Punjabi: return "Punjabi";
			case LocTextLanguage::Gujarati: return "Gujarati";
			case LocTextLanguage::Thai: return "Thai";
			case LocTextLanguage::Kannada: return "Kannada";
			case LocTextLanguage::Amharic: return "Amharic";
			case LocTextLanguage::Bhojpuri: return "Bhojpuri";
			case LocTextLanguage::MinNanChinese: return "Chinese, Min Nan";
			case LocTextLanguage::HakkaChinese: return "Chinese, Hakka";
			case LocTextLanguage::Burmese: return "Burmese";
			case LocTextLanguage::Yoruba: return "Yoruba";
			case LocTextLanguage::Polish: return "Polish";
			case LocTextLanguage::Ukrainian: return "Ukrainian";
			case LocTextLanguage::Malay: return "Malay";
			case LocTextLanguage::Oromo: return "Oromo";
			case LocTextLanguage::Maithili: return "Maithili";
			case LocTextLanguage::Sundanese: return "Sundanese";
			case LocTextLanguage::Romanian: return "Romanian";
			case LocTextLanguage::Dutch: return "Dutch";
			case LocTextLanguage::Nepali: return "Nepali";
			case LocTextLanguage::Saraiki: return "Saraiki";
			case LocTextLanguage::Awadhi: return "Awadhi";
			case LocTextLanguage::Azerbaijani: return "Azerbaijani";
			case LocTextLanguage::Greek: return "Greek";
			case LocTextLanguage::Hungarian: return "Hungarian";
			case LocTextLanguage::Czech: return "Czech";
			case LocTextLanguage::Swedish: return "Swedish";
			case LocTextLanguage::Belarusian: return "Belarusian";
			case LocTextLanguage::Bulgarian: return "Bulgarian";
			case LocTextLanguage::Serbian: return "Serbian";
			case LocTextLanguage::Croatian: return "Croatian";
			case LocTextLanguage::Slovak: return "Slovak";
			case LocTextLanguage::Danish: return "Danish";
			case LocTextLanguage::Finnish: return "Finnish";
			case LocTextLanguage::Norwegian: return "Norwegian";
			case LocTextLanguage::Hebrew: return "Hebrew";
			case LocTextLanguage::Armenian: return "Armenian";
			case LocTextLanguage::Georgian: return "Georgian";
			case LocTextLanguage::Kazakh: return "Kazakh";
			case LocTextLanguage::Uzbek: return "Uzbek";
			case LocTextLanguage::Turkmen: return "Turkmen";
			case LocTextLanguage::Kyrgyz: return "Kyrgyz";
			case LocTextLanguage::Tajik: return "Tajik";
			case LocTextLanguage::Mongolian: return "Mongolian";
			case LocTextLanguage::Pashto: return "Pashto";
			case LocTextLanguage::Kurdish: return "Kurdish";
			case LocTextLanguage::Sindhi: return "Sindhi";
			case LocTextLanguage::Sinhala: return "Sinhala";
			case LocTextLanguage::Khmer: return "Khmer";
			case LocTextLanguage::Lao: return "Lao";
			case LocTextLanguage::Assamese: return "Assamese";
			case LocTextLanguage::Odia: return "Odia";
			case LocTextLanguage::Malayalam: return "Malayalam";
			case LocTextLanguage::Konkani: return "Konkani";
			case LocTextLanguage::Sanskrit: return "Sanskrit";
			case LocTextLanguage::Filipino: return "Filipino";
			case LocTextLanguage::Tagalog: return "Tagalog";
			case LocTextLanguage::Cebuano: return "Cebuano";
			case LocTextLanguage::Ilocano: return "Ilocano";
			case LocTextLanguage::Hiligaynon: return "Hiligaynon";
			case LocTextLanguage::Malagasy: return "Malagasy";
			case LocTextLanguage::Zulu: return "Zulu";
			case LocTextLanguage::Xhosa: return "Xhosa";
			case LocTextLanguage::Afrikaans: return "Afrikaans";
			case LocTextLanguage::Shona: return "Shona";
			case LocTextLanguage::Kinyarwanda: return "Kinyarwanda";
			case LocTextLanguage::Kirundi: return "Kirundi";
			case LocTextLanguage::Lingala: return "Lingala";
			case LocTextLanguage::Wolof: return "Wolof";
			case LocTextLanguage::Fula: return "Fula";
			case LocTextLanguage::Igbo: return "Igbo";
			case LocTextLanguage::Akan: return "Akan";
			case LocTextLanguage::Somali: return "Somali";
			case LocTextLanguage::Tigrinya: return "Tigrinya";
			case LocTextLanguage::Berber: return "Berber";
			case LocTextLanguage::Luxembourgish: return "Luxembourgish";
			case LocTextLanguage::Icelandic: return "Icelandic";
			case LocTextLanguage::Irish: return "Irish";
			case LocTextLanguage::Welsh: return "Welsh";
			case LocTextLanguage::ScottishGaelic: return "Gaelic, Scots";
			case LocTextLanguage::Basque: return "Basque";
			case LocTextLanguage::Catalan: return "Catalan";
			case LocTextLanguage::Galician: return "Galician";
			case LocTextLanguage::Occitan: return "Occitan";
			case LocTextLanguage::Breton: return "Breton";
			case LocTextLanguage::Corsican: return "Corsican";
			case LocTextLanguage::Sardinian: return "Sardinian";
			case LocTextLanguage::Romansh: return "Romansh";
			case LocTextLanguage::Maltese: return "Maltese";
			case LocTextLanguage::Albanian: return "Albanian";
			case LocTextLanguage::Macedonian: return "Macedonian";
			case LocTextLanguage::Bosnian: return "Bosnian";
			case LocTextLanguage::Montenegrin: return "Montenegrin";
			case LocTextLanguage::Lithuanian: return "Lithuanian";
			case LocTextLanguage::Latvian: return "Latvian";
			case LocTextLanguage::Estonian: return "Estonian";
			case LocTextLanguage::Yiddish: return "Yiddish";
			case LocTextLanguage::Faroese: return "Faroese";
			case LocTextLanguage::NorthernSami: return "Northern Sami";
			case LocTextLanguage::Ladin: return "Ladin";
			case LocTextLanguage::Friulian: return "Friulian";
			case LocTextLanguage::Asturian: return "Asturian";
			case LocTextLanguage::Aragonese: return "Aragonese";
			case LocTextLanguage::Extremaduran: return "Extremaduran";
			case LocTextLanguage::Venetian: return "Venetian";
			case LocTextLanguage::Neapolitan: return "Neapolitan";
			case LocTextLanguage::Sicilian: return "Sicilian";
			case LocTextLanguage::Lombard: return "Lombard";
			case LocTextLanguage::Piedmontese: return "Piedmontese";
			case LocTextLanguage::Ligurian: return "Ligurian";
			case LocTextLanguage::EmilianRomagnol: return "Emilian Romagnol";
			case LocTextLanguage::Walloon: return "Walloon";
			case LocTextLanguage::Picard: return "Picard";
			case LocTextLanguage::FrancoProvencal: return "Franco Provencal";
			case LocTextLanguage::LowGerman: return "LowGerman";
			case LocTextLanguage::WestFrisian: return "Frisian, West";
			case LocTextLanguage::NorthFrisian: return "Frisian, North";
			case LocTextLanguage::SaterlandFrisian: return "Frisian, Saterland";
			case LocTextLanguage::Kashubian: return "Kashubian";
			case LocTextLanguage::Silesian: return "Silesian";
			case LocTextLanguage::Rusyn: return "Rusyn";
			case LocTextLanguage::UpperSorbian: return "Sorbian, Upper";
			case LocTextLanguage::LowerSorbian: return "Sorbian, Lower";
			case LocTextLanguage::Flemish: return "Flemish";
			case LocTextLanguage::SwissGerman: return "German, Swiss (CH)";
			case LocTextLanguage::Bavarian: return "Bavarian";
			case LocTextLanguage::Voro: return "Voro";
			case LocTextLanguage::Seto: return "Seto";
			case LocTextLanguage::Karelian: return "Karelian";
			case LocTextLanguage::Veps: return "Veps";
			case LocTextLanguage::Meankieli: return "Meankieli";
			case LocTextLanguage::Kven: return "Kven";
			case LocTextLanguage::CrimeanTatar: return "Crimean Tatar";
			case LocTextLanguage::Gagauz: return "Gagauz";
			case LocTextLanguage::Komi: return "Komi";
			case LocTextLanguage::Udmurt: return "Udmurt";
			case LocTextLanguage::Mari: return "Mari";
			case LocTextLanguage::Moksha: return "Moksha";
			case LocTextLanguage::Erzya: return "Erzya";
			case LocTextLanguage::Abkhaz: return "Abkhaz";
			case LocTextLanguage::Adyghe: return "Adyghe";
			case LocTextLanguage::Kabardian: return "Kabardian";
			case LocTextLanguage::Kumyk: return "Kumyk";
			case LocTextLanguage::Nogai: return "Nogai";
			case LocTextLanguage::Chuvash: return "Chuvash";
			case LocTextLanguage::Tatar: return "Tatar";
			case LocTextLanguage::Bashkir: return "Bashkir";
			case LocTextLanguage::Dari: return "Dari";
			case LocTextLanguage::IraqiArabic: return "Arabic, Iraqi";
			case LocTextLanguage::EgyptianArabic: return "Arabic, Egyptian";
			case LocTextLanguage::LevantineArabic: return "Arabic, Levantine";
			case LocTextLanguage::GulfArabic: return "Arabic, Gulf";
			case LocTextLanguage::MaghrebiArabic: return "Arabic, Maghrebi";
			case LocTextLanguage::NajdiArabic: return "Arabic, Najdi";
			case LocTextLanguage::ChadianArabic: return "Arabic, Chadian";
			case LocTextLanguage::AssyrianNeoAramaic: return "Assyrian Neo Aramaic";
			case LocTextLanguage::WesternNeoAramaic: return "Western Neo Aramaic";
			case LocTextLanguage::Syriac: return "Syriac";
			case LocTextLanguage::Coptic: return "Coptic";
			case LocTextLanguage::Domari: return "Domari";
			case LocTextLanguage::Luri: return "Luri";
			case LocTextLanguage::Balochi: return "Balochi";
			case LocTextLanguage::Gilaki: return "Gilaki";
			case LocTextLanguage::Mazanderani: return "Mazanderani";
			case LocTextLanguage::Ossetian: return "Ossetian";
			case LocTextLanguage::Chechen: return "Chechen";
			case LocTextLanguage::Ingush: return "Ingush";
			case LocTextLanguage::Avar: return "Avar";
			case LocTextLanguage::Lezgian: return "Lezgian";
			case LocTextLanguage::Sakha: return "Sakha";
			case LocTextLanguage::Uyghur: return "Uyghur";
			case LocTextLanguage::Dungan: return "Dungan";
			case LocTextLanguage::Burushaski: return "Burushaski";
			case LocTextLanguage::Wakhi: return "Wakhi";
			case LocTextLanguage::Shughni: return "Shughni";
			case LocTextLanguage::Yaghnobi: return "Yaghnobi";
			case LocTextLanguage::Khowar: return "Khowar";
			case LocTextLanguage::Balti: return "Balti";
			case LocTextLanguage::Rohingya: return "Rohingya";

				// South Asia
			case LocTextLanguage::Dogri: return "Dogri";
			case LocTextLanguage::Kashmiri: return "Kashmiri";
			case LocTextLanguage::Meitei: return "Meitei";
			case LocTextLanguage::Bodo: return "Bodo";
			case LocTextLanguage::Santali: return "Santali";
			case LocTextLanguage::Mundari: return "Mundari";
			case LocTextLanguage::Ho: return "Ho";
			case LocTextLanguage::Gondi: return "Gondi";
			case LocTextLanguage::Tulu: return "Tulu";
			case LocTextLanguage::Khasi: return "Khasi";
			case LocTextLanguage::Mizo: return "Mizo";
			case LocTextLanguage::Garo: return "Garo";
			case LocTextLanguage::Newar: return "Newar";
			case LocTextLanguage::Tamang: return "Tamang";
			case LocTextLanguage::Limbu: return "Limbu";
			case LocTextLanguage::Sherpa: return "Sherpa";
			case LocTextLanguage::Magahi: return "Magahi";
			case LocTextLanguage::Chhattisgarhi: return "Chhattisgarhi";
			case LocTextLanguage::Haryanvi: return "Haryanvi";
			case LocTextLanguage::Rajasthani: return "Rajasthani";
			case LocTextLanguage::Marwari: return "Marwari";
			case LocTextLanguage::Mewari: return "Mewari";
			case LocTextLanguage::Garhwali: return "Garhwali";
			case LocTextLanguage::Kumaoni: return "Kumaoni";
			case LocTextLanguage::Angika: return "Angika";
			case LocTextLanguage::Kurukh: return "Kurukh";
			case LocTextLanguage::Brahui: return "Brahui";
			case LocTextLanguage::Sylheti: return "Sylheti";
			case LocTextLanguage::Chittagonian: return "Chittagonian";
			case LocTextLanguage::Chakma: return "Chakma";
			case LocTextLanguage::Marma: return "Marma";
			case LocTextLanguage::Kokborok: return "Kokborok";
			case LocTextLanguage::Hindko: return "Hindko";
			case LocTextLanguage::PahariPothwari: return "Pahari Pothwari";
			case LocTextLanguage::BrajBhasha: return "Braj Bhasha";
			case LocTextLanguage::Bundeli: return "Bundeli";
			case LocTextLanguage::Bagheli: return "Bagheli";
			case LocTextLanguage::Ladakhi: return "Ladakhi";
			case LocTextLanguage::Kangri: return "Kangri";
			case LocTextLanguage::Mewati: return "Mewati";

			case LocTextLanguage::StandardChinese: return "Chinese, Standard";
			case LocTextLanguage::Shanghainese: return "Shanghainese";
			case LocTextLanguage::TaiwaneseHokkien: return "Taiwanese Hokkien";
			case LocTextLanguage::Fuzhounese: return "Fuzhounese";
			case LocTextLanguage::Teochew: return "Teochew";
			case LocTextLanguage::Hainanese: return "Hainanese";
			case LocTextLanguage::GanChinese: return "Chinese, Gan";
			case LocTextLanguage::XiangChinese: return "Chinese, Xiang";
			case LocTextLanguage::ClassicalChinese: return "Chinese, Classical";
			case LocTextLanguage::LiteraryChinese: return "Chinese, Literary";
			case LocTextLanguage::Hmong: return "Hmong";
			case LocTextLanguage::Zhuang: return "Zhuang";
			case LocTextLanguage::Yi: return "Yi";
			case LocTextLanguage::Naxi: return "Naxi";
			case LocTextLanguage::Dong: return "Dong";
			case LocTextLanguage::Kam: return "Kam";
			case LocTextLanguage::Bai: return "Bai";
			case LocTextLanguage::Manchu: return "Manchu";
			case LocTextLanguage::Ainu: return "Ainu";
			case LocTextLanguage::Okinawan: return "Okinawan";
			case LocTextLanguage::Jeju: return "Jeju";
			case LocTextLanguage::Tibetan: return "Tibetan";
			case LocTextLanguage::TibetanAmdo: return "Tibetan Amdo";
			case LocTextLanguage::TibetanKham: return "Tibetan Kham";
			case LocTextLanguage::Dzongkha: return "Dzongkha";

				// Southeast Asia
			case LocTextLanguage::Tetum: return "Tetum";
			case LocTextLanguage::Balinese: return "Balinese";
			case LocTextLanguage::Madurese: return "Madurese";
			case LocTextLanguage::Minangkabau: return "Minangkabau";
			case LocTextLanguage::Acehnese: return "Acehnese";
			case LocTextLanguage::Buginese: return "Buginese";
			case LocTextLanguage::Makassarese: return "Makassarese";
			case LocTextLanguage::Banjar: return "Banjar";
			case LocTextLanguage::Sasak: return "Sasak";
			case LocTextLanguage::Batak: return "Batak";
			case LocTextLanguage::Iban: return "Iban";
			case LocTextLanguage::KadazanDusun: return "Kadazan Dusun";
			case LocTextLanguage::Waray: return "Waray";
			case LocTextLanguage::Bikol: return "Bikol";
			case LocTextLanguage::Kapampangan: return "Kapampangan";
			case LocTextLanguage::Pangasinan: return "Pangasinan";
			case LocTextLanguage::Chamorro: return "Chamorro";
			case LocTextLanguage::HmongNjua: return "Hmong Njua";
			case LocTextLanguage::HmongDaw: return "Hmong Daw";
			case LocTextLanguage::Karen: return "Karen";
			case LocTextLanguage::SgawKaren: return "SgawKaren";
			case LocTextLanguage::PwoKaren: return "PwoKaren";
			case LocTextLanguage::Mon: return "Mon";
			case LocTextLanguage::Shan: return "Shan";
			case LocTextLanguage::Jingpho: return "Jingpho";
			case LocTextLanguage::Wa: return "Wa";
			case LocTextLanguage::Rejang: return "Rejang";
			case LocTextLanguage::Tausug: return "Tausug";
			case LocTextLanguage::Maranao: return "Maranao";
			case LocTextLanguage::Maguindanao: return "Maguindanao";
			case LocTextLanguage::Yakan: return "Yakan";
			case LocTextLanguage::Aklanon: return "Aklanon";
			case LocTextLanguage::KinarayA: return "KinarayA";

				// Africa
			case LocTextLanguage::Chichewa: return "Chichewa";
			case LocTextLanguage::Ndebele: return "Ndebele";
			case LocTextLanguage::NorthernNdebele: return "Ndebele, Northern";
			case LocTextLanguage::SouthernNdebele: return "Ndebele, Southern";
			case LocTextLanguage::Sesotho: return "Sesotho";
			case LocTextLanguage::NorthernSotho: return "Northern Sotho";
			case LocTextLanguage::Tswana: return "Tswana";
			case LocTextLanguage::Tsonga: return "Tsonga";
			case LocTextLanguage::Swati: return "Swati";
			case LocTextLanguage::Venda: return "Venda";
			case LocTextLanguage::Kikuyu: return "Kikuyu";
			case LocTextLanguage::Kamba: return "Kamba";
			case LocTextLanguage::Luo: return "Luo";
			case LocTextLanguage::Luhya: return "Luhya";
			case LocTextLanguage::Kalenjin: return "Kalenjin";
			case LocTextLanguage::Maasai: return "Maasai";
			case LocTextLanguage::Kanuri: return "Kanuri";
			case LocTextLanguage::Zarma: return "Zarma";
			case LocTextLanguage::Songhay: return "Songhay";
			case LocTextLanguage::Bambara: return "Bambara";
			case LocTextLanguage::Mandinka: return "Mandinka";
			case LocTextLanguage::Soninke: return "Soninke";
			case LocTextLanguage::Susu: return "Susu";
			case LocTextLanguage::Ewe: return "Ewe";
			case LocTextLanguage::Ga: return "Ga";
			case LocTextLanguage::Dagbani: return "Dagbani";
			case LocTextLanguage::Mossi: return "Mossi";
			case LocTextLanguage::Kabyle: return "Kabyle";
			case LocTextLanguage::Tamazight: return "Tamazight";
			case LocTextLanguage::Tachelhit: return "Tachelhit";
			case LocTextLanguage::Tarifit: return "Tarifit";
			case LocTextLanguage::Nubian: return "Nubian";
			case LocTextLanguage::Dinka: return "Dinka";
			case LocTextLanguage::Nuer: return "Nuer";
			case LocTextLanguage::Acholi: return "Acholi";
			case LocTextLanguage::Luganda: return "Luganda";
			case LocTextLanguage::Sango: return "Sango";
			case LocTextLanguage::Fang: return "Fang";
			case LocTextLanguage::Bemba: return "Bemba";
			case LocTextLanguage::Lozi: return "Lozi";
			case LocTextLanguage::Tumbuka: return "Tumbuka";
			case LocTextLanguage::Herero: return "Herero";
			case LocTextLanguage::Nama: return "Nama";
			case LocTextLanguage::Khoekhoe: return "Khoekhoe";
			case LocTextLanguage::CentralAtlasTamazight: return "Central Atlas Tamazight";
			case LocTextLanguage::Tamasheq: return "Tamasheq";
			case LocTextLanguage::HassaniyaArabic: return "Hassaniya Arabic";
			case LocTextLanguage::Tiv: return "Tiv";
			case LocTextLanguage::Edo: return "Edo";
			case LocTextLanguage::Ibibio: return "Ibibio";
			case LocTextLanguage::Efik: return "Efik";
			case LocTextLanguage::Ijaw: return "Ijaw";
			case LocTextLanguage::Urhobo: return "Urhobo";
			case LocTextLanguage::Itsekiri: return "Itsekiri";
			case LocTextLanguage::Nupe: return "Nupe";
			case LocTextLanguage::Gurene: return "Gurene";
			case LocTextLanguage::Dagaare: return "Dagaare";
			case LocTextLanguage::Kabye: return "Kabye";
			case LocTextLanguage::Temne: return "Temne";
			case LocTextLanguage::Mende: return "Mende";
			case LocTextLanguage::Krio: return "Krio";
			case LocTextLanguage::Afar: return "Afar";
			case LocTextLanguage::Sidamo: return "Sidamo";
			case LocTextLanguage::Wolaytta: return "Wolaytta";
			case LocTextLanguage::Gurage: return "Gurage";
			case LocTextLanguage::Tigre: return "Tigre";
			case LocTextLanguage::Beja: return "Beja";
			case LocTextLanguage::Fur: return "Fur";
			case LocTextLanguage::Zaghawa: return "Zaghawa";
			case LocTextLanguage::LubaKatanga: return "Luba Katanga";
			case LocTextLanguage::LubaKasai: return "Luba Kasai";
			case LocTextLanguage::Kituba: return "Kituba";
			case LocTextLanguage::Kongo: return "Kongo";
			case LocTextLanguage::Umbundu: return "Umbundu";
			case LocTextLanguage::Kimbundu: return "Kimbundu";

				// Indigenous languages of the Americas
			case LocTextLanguage::Navajo: return "Navajo";
			case LocTextLanguage::Cherokee: return "Cherokee";
			case LocTextLanguage::Cree: return "Cree";
			case LocTextLanguage::Inuktitut: return "Inuktitut";
			case LocTextLanguage::Inuvialuktun: return "Inuvialuktun";
			case LocTextLanguage::Ojibwe: return "Ojibwe";
			case LocTextLanguage::OjiCree: return "OjiCree";
			case LocTextLanguage::Blackfoot: return "Blackfoot";
			case LocTextLanguage::Lakota: return "Lakota";
			case LocTextLanguage::Dakota: return "Dakota";
			case LocTextLanguage::Mohawk: return "Mohawk";
			case LocTextLanguage::Mikmaq: return "Mikmaq";
			case LocTextLanguage::Innu: return "Innu";
			case LocTextLanguage::Yupik: return "Yupik";
			case LocTextLanguage::Tlingit: return "Tlingit";
			case LocTextLanguage::Haida: return "Haida";
			case LocTextLanguage::Quechua: return "Quechua";
			case LocTextLanguage::Aymara: return "Aymara";
			case LocTextLanguage::Guarani: return "Guarani";
			case LocTextLanguage::Nahuatl: return "Nahuatl";
			case LocTextLanguage::YucatecMaya: return "Yucatec Maya";
			case LocTextLanguage::Kiche: return "Kiche";
			case LocTextLanguage::Qeqchi: return "Qeqchi";
			case LocTextLanguage::Kaqchikel: return "Kaqchikel";
			case LocTextLanguage::Zapotec: return "Zapotec";
			case LocTextLanguage::Mixtec: return "Mixtec";
			case LocTextLanguage::Otomi: return "Otomi";
			case LocTextLanguage::Mapudungun: return "Mapudungun";
			case LocTextLanguage::Wayuu: return "Wayuu";
			case LocTextLanguage::Warao: return "Warao";
			case LocTextLanguage::Yanomami: return "Yanomami";
			case LocTextLanguage::ShipiboKonibo: return "Shipibo Konibo";
			case LocTextLanguage::Ashaninka: return "Ashaninka";
			case LocTextLanguage::Tupi: return "Tupi";
			case LocTextLanguage::Arawak: return "Arawak";
			case LocTextLanguage::Papiamento: return "Papiamento";
			case LocTextLanguage::HaitianCreole: return "Haitian Creole";
			case LocTextLanguage::JamaicanPatois: return "Jamaican Patois";
			case LocTextLanguage::SrananTongo: return "SrananTongo";
			case LocTextLanguage::LouisianaCreole: return "Louisiana Creole";
			case LocTextLanguage::Greenlandic: return "Greenlandic";
			case LocTextLanguage::Mam: return "Mam";
			case LocTextLanguage::Poqomchi: return "Poqomchi";
			case LocTextLanguage::Tzotzil: return "Tzotzil";
			case LocTextLanguage::Tzeltal: return "Tzeltal";
			case LocTextLanguage::Purepecha: return "Purepecha";
			case LocTextLanguage::Huastec: return "Huastec";
			case LocTextLanguage::Mazatec: return "Mazatec";
			case LocTextLanguage::Oodham: return "Oodham";
			case LocTextLanguage::Bribri: return "Bribri";
			case LocTextLanguage::Cabecar: return "Cabecar";
			case LocTextLanguage::Miskito: return "Miskito";
			case LocTextLanguage::Garifuna: return "Garifuna";
			case LocTextLanguage::Nheengatu: return "Nheengatu";
			case LocTextLanguage::Kichwa: return "Kichwa";
			case LocTextLanguage::Shuar: return "Shuar";
			case LocTextLanguage::Munduruku: return "Munduruku";
			case LocTextLanguage::Tikuna: return "Tikuna";
			case LocTextLanguage::HuniKuin: return "HuniKuin";

			case LocTextLanguage::YolnguMatha: return "Yolngu Matha";
			case LocTextLanguage::Warlpiri: return "Warlpiri";
			case LocTextLanguage::Pitjantjatjara: return "Pitjantjatjara";
			case LocTextLanguage::Arrernte: return "Arrernte";
			case LocTextLanguage::Tiwi: return "Tiwi";
			case LocTextLanguage::TorresStraitCreole: return "Torres Strait Creole";
			case LocTextLanguage::Maori: return "Maori";
			case LocTextLanguage::Rotuman: return "Rotuman";
			case LocTextLanguage::Kiribati: return "Kiribati";
			case LocTextLanguage::Tuvaluan: return "Tuvaluan";
			case LocTextLanguage::Nauruan: return "Nauruan";
			case LocTextLanguage::Niuean: return "Niuean";
			case LocTextLanguage::CookIslandsMaori: return "Cook Islands Maori";
			case LocTextLanguage::FijianHindi: return "Fijian Hindi";
			case LocTextLanguage::HiriMotu: return "Hiri Motu";
			case LocTextLanguage::Enga: return "Enga";
			case LocTextLanguage::WesternDani: return "Western Dani";
			case LocTextLanguage::Motu: return "Motu";
			case LocTextLanguage::Yapese: return "Yapese";
			case LocTextLanguage::Chuukese: return "Chuukese";
			case LocTextLanguage::Pohnpeian: return "Pohnpeian";
			case LocTextLanguage::Kosraean: return "Kosraean";
			case LocTextLanguage::Woleaian: return "Woleaian";

			default: return "Unknown";
		}
	}

	constexpr std::string LocTextLanguageVariantToString( LocTextLanguageVariant variant )
	{
		switch( variant )
		{
			default:
			case LocTextLanguageVariant::UnknownUnknown: return "und";

			case LocTextLanguageVariant::EnglishUnitedKingdom: return "en-gb";
			case LocTextLanguageVariant::EnglishUnitedStates: return "en-us";
			case LocTextLanguageVariant::EnglishCanada: return "en-ca";
			case LocTextLanguageVariant::EnglishAustralia: return "en-au";
			case LocTextLanguageVariant::EnglishNewZealand: return "en-nz";
			case LocTextLanguageVariant::EnglishIreland: return "en-ie";
			case LocTextLanguageVariant::EnglishSouthAfrica: return "en-za";
			case LocTextLanguageVariant::EnglishIndia: return "en-in";
			case LocTextLanguageVariant::EnglishSingapore: return "en-sg";
			case LocTextLanguageVariant::EnglishPhilippines: return "en-ph";

			case LocTextLanguageVariant::SpanishSpain: return "es-es";
			case LocTextLanguageVariant::SpanishMexico: return "es-mx";
			case LocTextLanguageVariant::SpanishArgentina: return "es-ar";
			case LocTextLanguageVariant::SpanishColombia: return "es-co";
			case LocTextLanguageVariant::SpanishChile: return "es-cl";
			case LocTextLanguageVariant::SpanishPeru: return "es-pe";
			case LocTextLanguageVariant::SpanishVenezuela: return "es-ve";
			case LocTextLanguageVariant::SpanishCaribbean: return "es-419";
			case LocTextLanguageVariant::SpanishCentralAmerica: return "es-419";
			case LocTextLanguageVariant::SpanishUnitedStates: return "es-us";

			case LocTextLanguageVariant::PortuguesePortugal: return "pt-pt";
			case LocTextLanguageVariant::PortugueseBrazil: return "pt-br";
			case LocTextLanguageVariant::PortugueseAngola: return "pt-ao";
			case LocTextLanguageVariant::PortugueseMozambique: return "pt-mz";

			case LocTextLanguageVariant::FrenchFrance: return "fr-fr";
			case LocTextLanguageVariant::FrenchCanada: return "fr-ca";
			case LocTextLanguageVariant::FrenchBelgium: return "fr-be";
			case LocTextLanguageVariant::FrenchSwitzerland: return "fr-ch";
			case LocTextLanguageVariant::FrenchWestAfrica: return "fr-011";
			case LocTextLanguageVariant::FrenchCaribbean: return "fr-029";

			case LocTextLanguageVariant::GermanGermany: return "de-de";
			case LocTextLanguageVariant::GermanAustria: return "de-at";
			case LocTextLanguageVariant::GermanSwitzerland: return "de-ch";
			case LocTextLanguageVariant::GermanLuxembourg: return "de-lu";

			case LocTextLanguageVariant::MandarinMainland: return "zh-cn";
			case LocTextLanguageVariant::MandarinTaiwan: return "zh-tw";
			case LocTextLanguageVariant::MandarinSingapore: return "zh-sg";
			case LocTextLanguageVariant::CantoneseHongKong: return "yue-hk";
			case LocTextLanguageVariant::CantoneseGuangzhou: return "yue-cn";
			case LocTextLanguageVariant::CantoneseMacau: return "yue-mo";
			case LocTextLanguageVariant::HokkienTaiwan: return "nan-tw";
			case LocTextLanguageVariant::HokkienSoutheastAsia: return "nan-sg";

			case LocTextLanguageVariant::ArabicModernStandard: return "ar-001";
			case LocTextLanguageVariant::ArabicEgyptian: return "ar-eg";
			case LocTextLanguageVariant::ArabicLevantine: return "apc";
			case LocTextLanguageVariant::ArabicGulf: return "ar-ae";
			case LocTextLanguageVariant::ArabicIraqi: return "ar-iq";
			case LocTextLanguageVariant::ArabicMaghrebi: return "ary";
			case LocTextLanguageVariant::ArabicSudanese: return "ar-sd";
			case LocTextLanguageVariant::ArabicYemeni: return "ar-ye";

			case LocTextLanguageVariant::PersianIran: return "fa-ir";
			case LocTextLanguageVariant::PersianDariAfghanistan: return "fa-af";
			case LocTextLanguageVariant::PersianTajikistan: return "tg-tj";

			case LocTextLanguageVariant::NorwegianBokmal: return "nb-no";
			case LocTextLanguageVariant::NorwegianNynorsk: return "nn-no";

			case LocTextLanguageVariant::SerbianCyrillic: return "sr-cyrl";
			case LocTextLanguageVariant::SerbianLatin: return "sr-latn";

			case LocTextLanguageVariant::KurdishKurmanji: return "kmr";
			case LocTextLanguageVariant::KurdishSorani: return "ckb";
			case LocTextLanguageVariant::KurdishSouthern: return "sdh";

			case LocTextLanguageVariant::MalayMalaysia: return "ms-my";
			case LocTextLanguageVariant::MalayBrunei: return "ms-bn";
			case LocTextLanguageVariant::MalaySingapore: return "ms-sg";
			case LocTextLanguageVariant::IndonesianStandard: return "id-id";

			case LocTextLanguageVariant::DutchNetherlands: return "nl-nl";
			case LocTextLanguageVariant::DutchBelgium: return "nl-be";

			case LocTextLanguageVariant::ItalianStandard: return "it-it";
			case LocTextLanguageVariant::ItalianSwitzerland: return "it-ch";

			case LocTextLanguageVariant::KoreanSouthKorea: return "ko-kr";
			case LocTextLanguageVariant::KoreanNorthKorea: return "ko-kp";

			case LocTextLanguageVariant::TamilIndia: return "ta-in";
			case LocTextLanguageVariant::TamilSriLanka: return "ta-lk";
			case LocTextLanguageVariant::TamilSingapore: return "ta-sg";

			case LocTextLanguageVariant::PunjabiEastern: return "pa-in";
			case LocTextLanguageVariant::PunjabiWestern: return "pa-pk";

			case LocTextLanguageVariant::QuechuaSouthern: return "qu";
			case LocTextLanguageVariant::QuechuaAncash: return "qva";
			case LocTextLanguageVariant::QuechuaCuscoCollao: return "quz";
		}
	}

}
