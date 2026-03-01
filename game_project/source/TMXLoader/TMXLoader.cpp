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
    // TMXMap::setMapSettings explicitly expects exactly this layout:
    // [0] version, [1] orientation, [2] renderOrder, [3] width
    // [4] height, [5] tileWidth, [6] tileHeight, [7] R, [8] G, [9] B
    std::vector<std::string> mapData(10, "0");
    mapData[1] = "orthogonal";
    mapData[2] = "right-down";

    // Safely assign attributes to their correct indices regardless of XML order
    for (rapidxml::xml_attribute<char>* attr = parentNode->first_attribute(); attr; attr = attr->next_attribute())
    {
        std::string attrName = attr->name();
        if (attrName == "version") mapData[0] = attr->value();
        else if (attrName == "orientation") mapData[1] = attr->value();
        else if (attrName == "renderorder") mapData[2] = attr->value();
        else if (attrName == "width") mapData[3] = attr->value();
        else if (attrName == "height") mapData[4] = attr->value();
        else if (attrName == "tilewidth") mapData[5] = attr->value();
        else if (attrName == "tileheight") mapData[6] = attr->value();
    }

    std::unordered_map<std::string, std::string> propertiesMap;
    loadProperties(propertiesMap, parentNode);

    // Pass the perfectly formatted array
    map->setMapSettings(mapData, propertiesMap);
}


void TMXLoader::loadTileSets(std::unique_ptr<TMXMap> const& map, rapidxml::xml_node<>* parentNode)
{
    rapidxml::xml_node<>* currentNode = parentNode->first_node("tileset");

    while (currentNode != nullptr)
    {
        std::unordered_map<std::string, std::string> tileSetData;
        std::unordered_map<std::string, std::string> propertiesMap;
        std::vector<TMXTile> tileVector;
        std::unordered_map<std::string, std::string> tileProperties;

        // 1. Safely read tileset attributes (firstgid, name, source, etc.)
        for (rapidxml::xml_attribute<char>* attr = currentNode->first_attribute(); attr; attr = attr->next_attribute())
        {
            tileSetData[attr->name()] = attr->value();
        }

        // 2. Safely check for tile offset
        rapidxml::xml_node<>* offsetNode = currentNode->first_node("tileoffset");
        if (offsetNode != nullptr)
        {
            if (offsetNode->first_attribute("x")) tileSetData["tileoffsetX"] = offsetNode->first_attribute("x")->value();
            if (offsetNode->first_attribute("y")) tileSetData["tileoffsetY"] = offsetNode->first_attribute("y")->value();
        }

        // 3. Load properties safely
        loadProperties(propertiesMap, currentNode);

        // 4. Safely check for the image node (If it's an external .tsx, this will safely be skipped!)
        rapidxml::xml_node<>* imageNode = currentNode->first_node("image");
        if (imageNode != nullptr)
        {
            for (rapidxml::xml_attribute<char>* attr = imageNode->first_attribute(); attr; attr = attr->next_attribute())
            {
                if (strcmp(attr->name(), "trans") == 0)
                {
                    try {
                        unsigned int colour = std::stoul(attr->value(), 0, 16);
                        tileSetData["red"] = std::to_string(colour / 0x10000);
                        tileSetData["green"] = std::to_string((colour / 0x100) % 0x100);
                        tileSetData["blue"] = std::to_string(colour % 0x100);
                    }
                    catch (...) {
                        tileSetData["red"] = "0"; tileSetData["green"] = "0"; tileSetData["blue"] = "0";
                    }
                }
                else
                {
                    tileSetData[attr->name()] = attr->value();
                }
            }
        }

        // 5. Safely read individual tile properties
        rapidxml::xml_node<>* tileNode = currentNode->first_node("tile");
        while (tileNode != nullptr)
        {
            unsigned int tileID = 0;
            if (tileNode->first_attribute("id")) {
                tileID = std::stoul(tileNode->first_attribute("id")->value());
            }

            tileProperties.clear();
            loadProperties(tileProperties, tileNode);
            tileVector.push_back(TMXTile(tileID, tileProperties));

            tileNode = tileNode->next_sibling("tile");
        }

        map->addTileSet(TMXTileSet(tileSetData, propertiesMap, tileVector));

        // Move to the next tileset
        currentNode = currentNode->next_sibling("tileset");
    }
}


void TMXLoader::loadLayers(std::unique_ptr<TMXMap> const& map, rapidxml::xml_node<>* parentNode)
{
    rapidxml::xml_node<>* currentNode = parentNode->first_node("layer");

    while (currentNode != nullptr)
    {
        std::unordered_map<std::string, std::string> layerProperties;
        std::string layerName = "Unknown";
        unsigned int layerWidth = 0;
        unsigned int layerHeight = 0;

        // SAFELY parse attributes by their actual name, ignoring order
        for (rapidxml::xml_attribute<char>* attr = currentNode->first_attribute(); attr; attr = attr->next_attribute())
        {
            std::string attrName = attr->name();
            if (attrName == "name") layerName = attr->value();
            else if (attrName == "width") layerWidth = std::stoul(attr->value());
            else if (attrName == "height") layerHeight = std::stoul(attr->value());
        }

        loadProperties(layerProperties, currentNode);

        // Safely size the 2D grid
        std::vector<std::vector<unsigned int>> tileVector;
        if (layerHeight > 0 && layerWidth > 0) {
            tileVector.resize(layerHeight, std::vector<unsigned int>(layerWidth, 0));
        }

        // Safely read the XML tile nodes
        // Safely read the XML tile nodes
        rapidxml::xml_node<>* dataNode = currentNode->first_node("data");
        if (dataNode != nullptr)
        {
            // --- Here are your missing variables! ---
            rapidxml::xml_node<>* tileNode = dataNode->first_node("tile");
            unsigned int currentTile = 0;
            unsigned int currentRow = 0;

            // --- Here is our newly fixed loop ---
            while (tileNode != nullptr && currentRow < layerHeight)
            {
                if (currentTile < layerWidth)
                {
                    unsigned int gid = 0; // Default to 0 (empty space)
                    rapidxml::xml_attribute<>* gidAttribute = tileNode->first_attribute("gid");

                    if (gidAttribute != nullptr)
                    {
                        // If the attribute exists, parse the number
                        gid = (unsigned int)std::stoul(gidAttribute->value());
                    }

                    // Assign the safely parsed gid to your 2D array
                    tileVector[currentRow][currentTile] = gid;

                    currentTile++;
                    tileNode = tileNode->next_sibling("tile");
                }
                else
                {
                    currentTile = 0;
                    currentRow++;
                }
            }
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


