//
//  TMXLoader.cpp
//  TMXLoader
//
//  Created by Marty on 06/09/2015.
//  Copyright (c) 2015 - 2020 Martin Grant. All rights reserved.
//  Available under MIT license. See License.txt for more information.
//
//  Uses RapidXML for file parsing.
//  Copyright (c) 2006, 2007 Marcin Kalicinski
//  http://rapidxml.sourceforge.net/
//  See /RapidXML/License.txt for more information.
//
//  www.midnightpacific.com
//  contact@midnightpacific.com
//  @_martingrant
//  http://bitbucket.org/martingrant/tmxloader
//

#include "TMXLoader.h"


TMXLoader::TMXLoader()
{
}


TMXLoader::~TMXLoader()
{
    m_mapContainer.clear();
    std::unordered_map<std::string, std::unique_ptr<TMXMap>>().swap(m_mapContainer);
}


void TMXLoader::loadMap(std::string mapName, std::string filePath)
{
    // String to hold file contents
    std::string fileContents = "";
    
    // Attempt to load file using provided file path
    bool fileLoaded = loadFile(filePath, fileContents);
    
    if (fileLoaded == true)
    {
        // Create new RapidXML document instance to use to parse map data
        rapidxml::xml_document<char> m_currentMap;
        m_currentMap.parse<0>((char*)fileContents.c_str());
        rapidxml::xml_node<> *parentNode = m_currentMap.first_node("map");
        
        // Add new TMXMap to m_mapContainer
        m_mapContainer[mapName] = std::unique_ptr<TMXMap>(new TMXMap());
        
        // Load the map settings, tilesets and layers
        loadMapSettings(m_mapContainer[mapName], parentNode);
        loadTileSets(m_mapContainer[mapName], parentNode);
        loadLayers(m_mapContainer[mapName], parentNode);
        
        std::cout << "TMXLoader: loaded map '" << mapName << "' from: '" << filePath << "' successfully" << std::endl;
    }
    else
    {
        std::cout << "TMXLoader: map '" << mapName << "' at '" << filePath << "' could not be loaded." << std::endl;
    }
}


TMXMap* TMXLoader::getMap(std::string mapName)
{
    // Attempt to find and return a map using provided name, else return nullptr
    
    std::unordered_map<std::string, std::unique_ptr<TMXMap>>::const_iterator iterator = m_mapContainer.find(mapName);
    
    if (iterator == m_mapContainer.end())
    {
        std::cout << "TMXLoader: map '" << mapName << "' not found." << std::endl;
    }
    else
    {
        return iterator->second.get();
    }
    
    return nullptr;
}


void TMXLoader::printMapData(std::string mapName)
{
    // Attempt to print data for a specific map
    
    std::unordered_map<std::string, std::unique_ptr<TMXMap>>::const_iterator iterator = m_mapContainer.find(mapName);
    
    if (iterator == m_mapContainer.end())
    {
        std::cout << "TMXLoader: map '" << mapName << "' not found." << std::endl;
    }
    else
    {
        iterator->second->printData();
    }
}


void TMXLoader::loadMapSettings(std::unique_ptr<TMXMap> const& map, rapidxml::xml_node<>* parentNode)
{
    // Safely extract all attributes into a temporary dictionary
    std::unordered_map<std::string, std::string> attrMap;
    for (rapidxml::xml_attribute<char>* attr = parentNode->first_attribute(); attr; attr = attr->next_attribute()) {
        attrMap[attr->name()] = attr->value();
    }

    // Build mapData strictly in the index order TMXMap.cpp expects
    std::vector<std::string> mapData(10, "0");
    mapData[0] = attrMap.count("version") ? attrMap["version"] : "1.0";
    mapData[1] = attrMap.count("orientation") ? attrMap["orientation"] : "orthogonal";
    mapData[2] = attrMap.count("renderorder") ? attrMap["renderorder"] : "right-down";
    mapData[3] = attrMap.count("width") ? attrMap["width"] : "0";
    mapData[4] = attrMap.count("height") ? attrMap["height"] : "0";
    mapData[5] = attrMap.count("tilewidth") ? attrMap["tilewidth"] : "0";
    mapData[6] = attrMap.count("tileheight") ? attrMap["tileheight"] : "0";

    // Parse background color safely into Red, Green, Blue
    unsigned int colour = 0; // Default black
    if (attrMap.count("backgroundcolor")) {
        std::string hex = attrMap["backgroundcolor"];
        if (hex.length() > 1 && hex[0] == '#') {
            try { colour = std::stoi(hex.substr(1), 0, 16); }
            catch (...) {}
        }
    }
    mapData[7] = std::to_string((colour >> 16) & 0xFF); // R
    mapData[8] = std::to_string((colour >> 8) & 0xFF);  // G
    mapData[9] = std::to_string(colour & 0xFF);         // B

    std::unordered_map<std::string, std::string> propertiesMap;
    loadProperties(propertiesMap, parentNode);

    map->setMapSettings(mapData, propertiesMap);
}


void TMXLoader::loadTileSets(std::unique_ptr<TMXMap> const &map, rapidxml::xml_node<> *parentNode)
{
	// Create a new node based on the parent node
	rapidxml::xml_node<> *currentNode = parentNode;

	// Check if there is a tileset node
	if (currentNode->first_node("tileset") != nullptr)
	{
		// Move to the first tileset node
		currentNode = currentNode->first_node("tileset");

		// Use a map to hold data for creating a tile set
		std::unordered_map<std::string, std::string> tileSetData;

		// Use a map to hold tileset properties
		std::unordered_map<std::string, std::string> propertiesMap;

		// Use a vector and map for individual tiles that have properties
		std::vector<TMXTile> tileVector;
		std::unordered_map<std::string, std::string> tileProperties;

		// Loop whilst there are tileset nodes found
		while (currentNode != nullptr)
		{
			// Read tileset data 
			for (rapidxml::xml_attribute<char> *attr = currentNode->first_attribute(); attr; attr = attr->next_attribute())
			{
				tileSetData[attr->name()] = attr->value();
			}

			// Check for offset node
			rapidxml::xml_node<> *offsetNode = currentNode;
			if (offsetNode->first_node("tileoffset") != nullptr)
			{
				offsetNode = offsetNode->first_node("tileoffset");

				tileSetData["tileoffsetX"] = offsetNode->first_attribute()->value();
				tileSetData["tileoffsetY"] = offsetNode->first_attribute()->next_attribute()->value();
			}

			// Clear the properties map of the data from the previous tileset
			propertiesMap.clear();
			// Load tileset properties
			loadProperties(propertiesMap, currentNode);

			// Move to the image childnode and read data
			currentNode = currentNode->first_node("image");
			for (rapidxml::xml_attribute<char> *attr = currentNode->first_attribute(); attr; attr = attr->next_attribute())
			{
				if (strcmp(attr->name(), "trans") == 0)
				{
					unsigned int colour = std::stoi(attr->value(), 0, 16);
                    
                    // Convert from hex to RGB
					tileSetData["red"] = std::to_string(colour / 0x10000);
					tileSetData["green"] = std::to_string((colour / 0x100) % 0x100);
					tileSetData["blue"] = std::to_string(colour / 0x10000);
				}
				else
				{
					tileSetData[attr->name()] = attr->value();
				}
			}

			// Check and load if any individual tiles have properties
			tileVector.clear();
			rapidxml::xml_node<> *tileNode = currentNode->parent()->first_node("tile");
			while (tileNode != nullptr)
			{
				unsigned int tileID = atoi(tileNode->first_attribute()->value());
				loadProperties(tileProperties, tileNode);
				tileVector.push_back(TMXTile(tileID, tileProperties));

				tileProperties.clear();

				if (tileNode->next_sibling("tile") != nullptr)
				{
					tileNode = tileNode->next_sibling("tile");
				}
				else
				{
					break;
				}
			}

			// Pass the new tileset data to the map
 			map->addTileSet(TMXTileSet(tileSetData, propertiesMap, tileVector));

			// Move to the next tileset node and increment the counter
			if (currentNode->parent()->next_sibling("tileset") == nullptr)
			{
				break;
			}
			else
			{
				tileSetData.clear();
				currentNode = currentNode->parent()->next_sibling("tileset");
			}
		}
	}
}


void TMXLoader::loadLayers(std::unique_ptr<TMXMap> const& map, rapidxml::xml_node<>* parentNode)
{
    rapidxml::xml_node<>* currentNode = parentNode->first_node("layer");

    while (currentNode != nullptr)
    {
        std::unordered_map<std::string, std::string> layerProperties;
        std::unordered_map<std::string, std::string> attrMap;

        // Safely extract attributes by name
        for (rapidxml::xml_attribute<char>* attr = currentNode->first_attribute(); attr; attr = attr->next_attribute()) {
            attrMap[attr->name()] = attr->value();
        }

        std::string layerName = attrMap.count("name") ? attrMap["name"] : "Layer";
        unsigned int layerWidth = attrMap.count("width") ? std::stoul(attrMap["width"]) : 0;
        unsigned int layerHeight = attrMap.count("height") ? std::stoul(attrMap["height"]) : 0;

        loadProperties(layerProperties, currentNode);

        // Access the <data> and first <tile> node safely
        rapidxml::xml_node<>* dataNode = currentNode->first_node("data");
        rapidxml::xml_node<>* tileNode = dataNode ? dataNode->first_node("tile") : nullptr;

        std::vector<std::vector<unsigned int>> tileVector(layerHeight, std::vector<unsigned int>(layerWidth, 0));

        unsigned int currentTile = 0;
        int currentRow = 0;

        // Loop whilst there are still tiles to be read
        while (tileNode != nullptr)
        {
            if (currentTile < layerWidth && currentRow < layerHeight)
            {
                // FIX: Look specifically for the "gid" attribute. If the tag is just <tile/>, it will safely be 0.
                rapidxml::xml_attribute<char>* gidAttr = tileNode->first_attribute("gid");
                if (gidAttr != nullptr) {
                    try {
                        tileVector[currentRow][currentTile] = (unsigned int)std::stoul(gidAttr->value());
                    }
                    catch (...) {
                        tileVector[currentRow][currentTile] = 0; // Fallback for invalid numbers
                    }
                }
                else {
                    tileVector[currentRow][currentTile] = 0; // Empty tile
                }

                currentTile++;
            }

            // Grid wrap-around logic
            if (currentTile >= layerWidth) {
                currentTile = 0;
                currentRow++;
            }

            tileNode = tileNode->next_sibling("tile");
        }

        map->addLayer(TMXTileLayer(layerName, layerWidth, layerHeight, layerProperties, tileVector));

        currentNode = currentNode->next_sibling("layer");
    }
}


void TMXLoader::loadProperties(std::unordered_map<std::string, std::string>& propertiesMap, rapidxml::xml_node<> *parentNode)
{
	// Create a new node based on the parent node
	rapidxml::xml_node<> *currentNode = parentNode;

	// Check if there is a properties node
	if (currentNode->first_node("properties") != nullptr)
	{
		// Move to the properties node
		currentNode = currentNode->first_node("properties");
		// Move to the first property node
		currentNode = currentNode->first_node("property");

		// Loop whilst there are property nodes found
		while (currentNode != nullptr)
		{
			propertiesMap[currentNode->first_attribute()->value()] = currentNode->first_attribute()->next_attribute()->value();
			currentNode = currentNode->next_sibling("property");
		}
	}
}


bool TMXLoader::loadFile(std::string filePath, std::string &fileContents)
{
    std::ifstream file(filePath, std::ios::in | std::ios::binary);

    if (file)
    {
        file.seekg(0, std::ios::end);
        fileContents.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(&fileContents[0], fileContents.size());
        file.close();
        
        return true;
    }
    return false;
}


