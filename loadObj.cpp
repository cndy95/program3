////////////////////////////////////////////////////////////////////
//	loadObj.cpp
////////////////////////////////////////////////////////////////////

#include "loadObj.h"



//////////////////////////////////////////////////////////////////////////
//	initialize an objInfo record
//////////////////////////////////////////////////////////////////////////
void initObjInfo(objInfo& objRef)
{
	objRef.VAO = 0;
	objRef.VAOsize = 0;
	objRef.bounds[0] = objRef.bounds[2] = objRef.bounds[4] = FLT_MAX;
	objRef.bounds[1] = objRef.bounds[3] = objRef.bounds[5] = FLT_MIN;
	objRef.modelMatrix = mat4(1.f);
	objRef.shader = BLINN_PHONG;
	strcpy(objRef.materialName, "");
	objRef.Ka = vec3( .1f, .1f, .1f );
	objRef.Kd = vec3( .9f, .9f, .9f );
	objRef.Ks = vec3( 0.f, 0.f, 0.f );
	objRef.n = 60.f;
	strcpy(objRef.diffuseTex, "");
	objRef.diffuseTexMapID = -1;
	strcpy(objRef.bumpMap, "");
	objRef.bumpMapTexID = -1;
}

//////////////////////////////////////////////////////////////////////////
//	loadObjFile
//		input:  fileName - character string with the name of the OBJ file
//		output:  theObjects - pointer to an array of OBJ descriptions in objInfo struct
//		output:  numObjects - a count of the number of objects in the OBJ file
//////////////////////////////////////////////////////////////////////////
int loadObjFile(char *fileName, objInfo **theObjects, int *numObjects)
{
	bool doingTextureMaps = true;		// not doing texture maps at this point

	ifstream objFile;
	char keyword[32];
	char *token, *token2;
	char line[1024];
	int totalVertexCount = 0;
	bool computeNormals = false;
	int facesCount = 0;
	int verticesCount = 0;
	char currentMaterialName[32];
	float fileBounds[6];	// bounds of the entire OBJ file
	fileBounds[0] = fileBounds[2] = fileBounds[4] = FLT_MAX;
	fileBounds[1] = fileBounds[3] = fileBounds[5] = FLT_MIN;

	strcpy(currentMaterialName, "default");

	vector<objInfo> objList;	// the vector of object descriptions from this OBJ file

	colorValues tempColor;
	vector<colorValues> colorTable;		// list of color descriptions found in material file

	GLuint vertexPosition, vertexNormal, vertexTextureIndices;

	// initialize currentObj with defaults for material (color), texture maps, etc
	objInfo currentObj;	// currently active obj, including information specified with a usemtl line
	initObjInfo(currentObj);

	bool firstColor = true;	// first color found in mtllib
	bool firstMaterial = true;	// first usemtl line in OBJ file
	objInfo tempObj;	// temporary object to hold information being read from file

	objFile.open(fileName, ios::in);
	if (!objFile.is_open())
	{
		cerr << "unable to read obj file: " << fileName << endl;
		cerr << "program is exiting" << endl;
		exit(3);
	}

	cout << "Reading obj file " << fileName << endl;

	// get path of objFileName
	char pathToFile[256];       // path to input obj file
	char *pch, *pchTemp, *pchTemp2;
	pchTemp = fileName;
	pchTemp2 = pathToFile;
	pch = strrchr(fileName, '/');
	if (pch != NULL)
	{
		while (pchTemp != pch)
			*(pchTemp2++) = *(pchTemp++);
		*(pchTemp2++) = '/';
		*(pchTemp2) = '\0';
	}
	else
		strcpy(pathToFile, "");

	// we have been able to open a wavefront obj file, so set up the first VAO for this file; there may be multiple VAOs
	// each usemtl line will trigger a new VAO, except the first one.  And there may not be any usemtl lines in an OBJ file
	glGenVertexArrays(1, &(currentObj.VAO));
	glBindVertexArray(currentObj.VAO);

	three tempFloats;

	vector <three> vertices;	// these three arrays hold the values read from the obj file
	vector <three> textureIndices;
	vector <three> normals;

	vector<float> vPosition;	// these arrays hold the values to be transferred to OpenGL buffers
	vector<float> vNormal;
	vector<float> vTextureIndex;

	while (!objFile.eof())
	{
		objFile >> keyword;

		if (strcmp(keyword, "mtllib") == 0)	// material file name
		{
			objFile >> keyword;	// name of material file
			// need to process the material file, hmmm...
			objFile.getline(line, 1024);
			char fullPath[128];
			strcpy(fullPath, pathToFile);
			strcat(fullPath, keyword);

			ifstream mtlFile;
			mtlFile.open(fullPath, ios::in);
			if (!mtlFile.is_open())
			{
				cerr << "could not read mtllib " << fullPath << endl;
				cerr << "default color is white " << endl;
			}
			else
			{
				while (!mtlFile.eof())
				{
					mtlFile >> keyword;
					if (strcmp(keyword, "newmtl") == 0)
					{
						if (firstColor)
							firstColor = false;
						else
							colorTable.push_back(tempColor);

						mtlFile >> tempColor.name;	// read this color description and store in colorTable
						tempColor.ambient = vec3( .1f, .1f, .1f );
						tempColor.diffuse = vec3(.9f, .9f, .9f );
						tempColor.specular = vec3( 1.f, 1.f, 1.f );
						tempColor.shininess = 10.f;
						strcpy(tempColor.diffuseTexMap, "");
						strcpy(tempColor.bumpTexMap, "");
						
					}
					else if (strcmp(keyword, "Ka") == 0)
					{
						mtlFile >> tempColor.ambient.r;
						mtlFile >> tempColor.ambient.g;
						mtlFile >> tempColor.ambient.b;
					}
					else if (strcmp(keyword, "Kd") == 0)
					{
						mtlFile >> tempColor.diffuse.r;
						mtlFile >> tempColor.diffuse.g;
						mtlFile >> tempColor.diffuse.b;
					}
					else if (strcmp(keyword, "Ks") == 0)
					{
						mtlFile >> tempColor.specular.r;
						mtlFile >> tempColor.specular.g;
						mtlFile >> tempColor.specular.b;
					}
					else if (strcmp(keyword, "Ns") == 0)
					{
						mtlFile >> tempColor.shininess;
					}
					else if (strcmp(keyword, "map_Kd") == 0)
					{
						mtlFile >> tempColor.diffuseTexMap;
					}
					else if (strcmp(keyword, "map_Bump") == 0)
					{
						mtlFile >> tempColor.bumpTexMap;
					}
					else // unrecognized token, so just skip the line
					{
						mtlFile.getline(line, 1024);
					}
				}
				colorTable.push_back(tempColor);
			}
		}
		else if (strcmp(keyword, "usemtl") == 0)
		{
			objFile >> keyword;		// name of selected color table entry
			// search table to find entry
			for (unsigned int i = 0; i < colorTable.size(); i++)
			{
				if (strcmp(keyword, colorTable[i].name) == 0)
				{
					strcpy(currentMaterialName, colorTable[i].name);
					if (firstMaterial)
					{
						firstMaterial = false;
					}
					else
					{
						// complete the current VAO, save it to the vector array
						currentObj.VAOsize = totalVertexCount;	// polygon count for this part of OBJ file
						
						objList.push_back(currentObj);

						glBindVertexArray(currentObj.VAO);	// ensure VAO is current
						// create OpenGL buffers for vertices and normals
						glGenBuffers(1, &vertexPosition);
						glGenBuffers(1, &vertexNormal);
						glGenBuffers(1, &vertexTextureIndices);

						glBindBuffer(GL_ARRAY_BUFFER, vertexPosition);
						glBufferData(GL_ARRAY_BUFFER, vPosition.size() * sizeof(float), &vPosition[0], GL_STATIC_DRAW);
						glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
						glEnableVertexAttribArray(0);

						glBindBuffer(GL_ARRAY_BUFFER, vertexNormal);
						glBufferData(GL_ARRAY_BUFFER, vNormal.size() * sizeof(float), &vNormal[0], GL_STATIC_DRAW);
						glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
						glEnableVertexAttribArray(1);

						if (vTextureIndex.size() > 0)
						{
							glBindBuffer(GL_ARRAY_BUFFER, vertexTextureIndices);
							glBufferData(GL_ARRAY_BUFFER, vTextureIndex.size() * sizeof(float), &vTextureIndex[0], GL_STATIC_DRAW);
							glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
							glEnableVertexAttribArray(2);
						}

						// prepare for next VAO
						totalVertexCount = 0;
						vPosition.clear();
						vNormal.clear();
						vTextureIndex.clear();
						initObjInfo(currentObj);
						glGenVertexArrays(1, &(currentObj.VAO));
						glBindVertexArray(currentObj.VAO);
					}

					// set the selected material description to the current obj description
					currentObj.Ka = colorTable[i].ambient;
					currentObj.Kd = colorTable[i].diffuse;
					currentObj.Ks = colorTable[i].specular;
					currentObj.n = colorTable[i].shininess;
					strcpy(currentObj.materialName, colorTable[i].name);
					strcpy(currentObj.diffuseTex, colorTable[i].diffuseTexMap);
					strcpy(currentObj.bumpMap, colorTable[i].bumpTexMap);

					break;	// stop with the first one found
				}
			}
		}
		else if (strcmp(keyword, "v") == 0)	// we have a vertex
		{
			// assume three more values, the x,y,z coordinates
			objFile >> tempFloats.x >> tempFloats.y >> tempFloats.z;
			objFile.getline(line, 128);
			vertices.push_back(tempFloats);
			verticesCount++;
			fileBounds[0] = tempFloats.x < fileBounds[0] ? tempFloats.x : fileBounds[0];
			fileBounds[1] = tempFloats.x > fileBounds[1] ? tempFloats.x : fileBounds[1];
			fileBounds[2] = tempFloats.y < fileBounds[2] ? tempFloats.y : fileBounds[2];
			fileBounds[3] = tempFloats.y > fileBounds[3] ? tempFloats.y : fileBounds[3];
			fileBounds[4] = tempFloats.z < fileBounds[4] ? tempFloats.z : fileBounds[4];
			fileBounds[5] = tempFloats.z > fileBounds[5] ? tempFloats.z : fileBounds[5];
		}
		else if (strcmp(keyword, "vt") == 0)	// texture map indices
		{
			objFile >> tempFloats.x >> tempFloats.y;
			tempFloats.z = -1.0f;
			textureIndices.push_back(tempFloats);
		}
		else if (strcmp(keyword, "vn") == 0)
		{
			objFile >> tempFloats.x >> tempFloats.y >> tempFloats.z;
			normals.push_back(tempFloats);
		}
		else if (strcmp(keyword, "f") == 0)	// a face or triangle description
		{
			// f  and first reference is vertex id, second is texture id, and third is normal id
			// second and third ids are optional   
			// it is possible to have two // together, such as 3//1
			objFile.getline(line, 1024);
			// break out the information of indices for vertices, textures and normals
			token = strtok(line, " ");

			// maintain a list of indices into the vertices for this face
			int numVertices = 0;
			int vertexIndices[20];	// better not have this many!
			int indices[3];
			char number[16];

			facesCount++;
			if (facesCount % 10000 == 0)	// command line progress meter
				cout << "\t faces " << facesCount << endl;

			while (token != NULL)
			{
				// process one group of numbers

				int charCount;
				int indexCount = 0;
				int totalCharCount = 0;
				// copy characters into temp until '/' or blank is found
				token2 = token;		// set up start of search
				int len = strlen(token);
				indices[0] = indices[1] = indices[2] = -1;
				// first index is mandatory
				while (totalCharCount < len && token2 != NULL)
				{
					charCount = 0;
					while (*token2 != '/' && totalCharCount < len)
					{
						number[charCount++] = *token2;
						token2++;
						totalCharCount++;
					}
					number[charCount] = '\0';	// add trailing NULL
					if (totalCharCount < len)
					{
						token2++;	// move pointer past /
						totalCharCount++;
					}
					indices[indexCount++] = atoi(number);
					if (totalCharCount < len && *token2 == '/') // two slashes in a row
					{
						token2++;
						totalCharCount++;
						indices[indexCount++] = -1;	// place holder for no texture id
					}
				}

				// save the vertex indices as we process them
				vertexIndices[numVertices++] = indices[0];

				// take this 'face' description and put values into the app. arrays that will be used to
				// fill the OpenGL buffers

				// store the position information into app buffer
				vPosition.push_back(vertices[indices[0] - 1].x);
				vPosition.push_back(vertices[indices[0] - 1].y);
				vPosition.push_back(vertices[indices[0] - 1].z);

				totalVertexCount++;		// total number of vertices into this app buffer

				// store the texture indices if specified
				if (indices[1] != -1)
				{
					vTextureIndex.push_back(textureIndices[indices[1] - 1].x);
					vTextureIndex.push_back(textureIndices[indices[1] - 1].y);
				}

				// store the normal information if specified
				if (indices[2] != -1)
				{
					vNormal.push_back(normals[indices[2] - 1].x);
					vNormal.push_back(normals[indices[2] - 1].y);
					vNormal.push_back(normals[indices[2] - 1].z);
					computeNormals = false;
				}
				else
				{
					computeNormals = true;
				}

				token = strtok(NULL, " ");
			}	// processing all tokens

			// if normals were not specified, then compute one now

			if (computeNormals)
			{
				float v1x = vertices[vertexIndices[1] - 1].x - vertices[vertexIndices[0] - 1].x;
				float v1y = vertices[vertexIndices[1] - 1].y - vertices[vertexIndices[0] - 1].y;
				float v1z = vertices[vertexIndices[1] - 1].z - vertices[vertexIndices[0] - 1].z;
				float v2x = vertices[vertexIndices[2] - 1].x - vertices[vertexIndices[0] - 1].x;
				float v2y = vertices[vertexIndices[2] - 1].y - vertices[vertexIndices[0] - 1].y;
				float v2z = vertices[vertexIndices[2] - 1].z - vertices[vertexIndices[0] - 1].z;
				vec3 v1(v1x, v1y, v1z);
				vec3 v2(v2x, v2y, v2z);
				vec3 norm = cross(v1, v2);
				norm = normalize(norm);

				for (int i = 0; i<numVertices; i++)
				{
					vNormal.push_back(norm[0]);
					vNormal.push_back(norm[1]);
					vNormal.push_back(norm[2]);
				}
			}
		}	// reading f description
		else // unrecognized token, so just skip it
		{
			objFile.getline(line, 1024);
		}
	}	// while loop reading entire obj file

	// take care of the final VAO
	currentObj.VAOsize = totalVertexCount;
	strcpy(currentObj.materialName, currentMaterialName);

	objList.push_back(currentObj);

	glBindVertexArray(currentObj.VAO);

	// create OpenGL buffers for vertices and normals
	glGenBuffers(1, &vertexPosition);
	glGenBuffers(1, &vertexNormal);
	glGenBuffers(1, &vertexTextureIndices);

	glBindBuffer(GL_ARRAY_BUFFER, vertexPosition);
	glBufferData(GL_ARRAY_BUFFER, vPosition.size() * sizeof(float), &vPosition[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vertexNormal);
	glBufferData(GL_ARRAY_BUFFER, vNormal.size() * sizeof(float), &vNormal[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	if (vTextureIndex.size() > 0 && doingTextureMaps)		// disabled for assignment 2
	{
		glBindBuffer(GL_ARRAY_BUFFER, vertexTextureIndices);
		glBufferData(GL_ARRAY_BUFFER, vTextureIndex.size() * sizeof(float), &vTextureIndex[0], GL_STATIC_DRAW);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);
	}

		// print out information on this obj file
	cout << "  bounds of the file: " << fileBounds[0] << " x " << fileBounds[1] << " "
		<< fileBounds[2] << " y " << fileBounds[3] << " "
		<< fileBounds[4] << " z " << fileBounds[5] << endl;

	// prepare return variables
	*numObjects = objList.size();
	objInfo *tempObjList;
	tempObjList = (objInfo *)malloc(sizeof(objInfo)* (*numObjects));
	for (int i = 0; i < (int)objList.size(); i++)
	{
		for (int j = 0; j < 6; j++)
			objList[i].bounds[j] = fileBounds[j];
		tempObjList[i] = objList[i];
	}
	*theObjects = tempObjList;
		
	return 1;
}