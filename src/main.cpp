#define NUM_OBJECTS 2

#include <iostream>
#include <string>
#include <assert.h>

#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

//GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//STB_IMAGE
#include <stb_image/stb_image.h>

using namespace glm;

#include <cmath>

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
int loadSimpleOBJ(string filePath, int &nVertices, vec3 color);
GLuint loadTexture(string filePath, int &width, int &height);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar* vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texc;

uniform mat4 projection;
uniform mat4 model;

out vec2 texCoord;
out vec3 vNormal;
out vec4 fragPos; 
out vec4 vColor;
void main()
{
   	gl_Position = projection * model * vec4(position.x, position.y, position.z, 1.0);
	fragPos = model * vec4(position.x, position.y, position.z, 1.0);
	texCoord = texc;
	vNormal = normal;
	vColor = vec4(color,1.0);
})";

//Códifo fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar* fragmentShaderSource = R"(
#version 400
in vec2 texCoord;
uniform sampler2D texBuff;
uniform vec3 lightPos;
uniform vec3 camPos;
uniform float ka;
uniform float kd;
uniform float ks;
uniform float q;
out vec4 color;
in vec4 fragPos;
in vec3 vNormal;
in vec4 vColor;
void main()
{
	vec3 lightColor = vec3(1.0,1.0,1.0);
	//vec4 objectColor = texture(texBuff,texCoord);
	vec4 objectColor = vColor;

	//Coeficiente de luz ambiente
	vec3 ambient = ka * lightColor;

	//Coeficiente de reflexão difusa
	vec3 N = normalize(vNormal);
	vec3 L = normalize(lightPos - vec3(fragPos));
	float diff = max(dot(N, L),0.0);
	vec3 diffuse = kd * diff * lightColor;

	//Coeficiente de reflexão especular
	vec3 R = normalize(reflect(-L,N));
	vec3 V = normalize(camPos - vec3(fragPos));
	float spec = max(dot(R,V),0.0);
	spec = pow(spec,q);
	vec3 specular = ks * spec * lightColor; 

	vec3 result = (ambient + diffuse) * vec3(objectColor) + specular;
	color = vec4(result,1.0);
})";

bool rotateX=false, rotateY=false, rotateZ=false;

struct Object
{
	GLuint VAO; //Índice do buffer de geometria
	int nVertices; //nro de vértices
	mat4 model; //matriz de transformações do objeto
	float offsetX = 0.0f;
	float offsetY = 0.0f;
	float offsetZ = 0.0f;
	float scale = 1.0f;
};

Object objects[NUM_OBJECTS];
int selectedObject = 0;

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	// Criação da janela GLFW
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Atividade Vivencial 2", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	// Obtendo as informações de versão
	const GLubyte* renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	vec3 colors[NUM_OBJECTS] = {
		vec3(1.0, 0.0, 0.0),
		vec3(1.0, 1.0, 0.0),
	};

	for (size_t i = 0; i < NUM_OBJECTS; i++)
	{
		objects[i].VAO = loadSimpleOBJ("../Modelos3D/Suzanne.obj", objects[i].nVertices, colors[i]);
	}

	objects[1].offsetX = 3.0f;

	// Carregando uma textura e armazenando seu id
	int imgWidth, imgHeight;
	GLuint texID = loadTexture("../assets/tex/pixelWall.png",imgWidth,imgHeight);

	float ka = 0.1, kd  =0.5, ks = 0.5, q = 10.0;
	vec3 lightPos = vec3(0.6, 1.2, -0.5);
	vec3 camPos = vec3(1.5,0.0,10.0);

	glUseProgram(shaderID);

	// Enviar a informação de qual variável armazenará o buffer da textura
	glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);

	glUniform1f(glGetUniformLocation(shaderID, "ka"), ka);
	glUniform1f(glGetUniformLocation(shaderID, "kd"), kd);
	glUniform1f(glGetUniformLocation(shaderID, "ks"), ks);
	glUniform1f(glGetUniformLocation(shaderID, "q"), q);
	glUniform3f(glGetUniformLocation(shaderID, "lightPos"), lightPos.x,lightPos.y,lightPos.z);
	glUniform3f(glGetUniformLocation(shaderID, "camPos"), camPos.x,camPos.y,camPos.z);

	//Ativando o primeiro buffer de textura da OpenGL
	glActiveTexture(GL_TEXTURE0);

	// Matriz de projeção paralela ortográfica
	// mat4 projection = ortho(-10.0, 10.0, -10.0, 10.0, -1.0, 1.0);
	mat4 projection = ortho(-1.0, 1.0, -1.0, 1.0, -3.0, 3.0);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

	// Matriz de modelo: transformações na geometria (objeto)
	mat4 model = mat4(1); // matriz identidade
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); //cor de fundo
		glClear(GL_COLOR_BUFFER_BIT);

		float angle = (GLfloat)glfwGetTime();

		for (size_t i = 0; i < NUM_OBJECTS; i++)
		{
			objects[i].model = mat4(1); //matriz identidade
			objects[i].model = scale(objects[i].model, vec3(objects[i].scale, objects[i].scale, objects[i].scale));
			objects[i].model = translate(objects[i].model, vec3(objects[i].offsetX, objects[i].offsetY, objects[i].offsetZ));

			if (rotateX)
			{
				objects[selectedObject].model = rotate(objects[selectedObject].model, angle, vec3(1.0f, 0.0f, 0.0f));
			}
			else if (rotateY)
			{
				objects[selectedObject].model = rotate(objects[selectedObject].model, angle, vec3(0.0f, 1.0f, 0.0f));
			}
			else if (rotateZ)
			{
				objects[selectedObject].model = rotate(objects[selectedObject].model, angle, vec3(0.0f, 0.0f, 1.0f));
			}

			glBindVertexArray(objects[i].VAO);
			glBindTexture(GL_TEXTURE_2D, texID);
			
			glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(objects[i].model));
			glDrawArrays(GL_TRIANGLES, 0, objects[i].nVertices);	
		}
		
		glBindVertexArray(0); // Desconectando o buffer de geometria

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}

	for (size_t i = 0; i < NUM_OBJECTS; i++)
	{
		// Pede pra OpenGL desalocar os buffers
		glDeleteVertexArrays(1, &objects[i].VAO);
	}

	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_X && action == GLFW_PRESS)
	{
		rotateX = true;
		rotateY = false;
		rotateZ = false;
	}

	if (key == GLFW_KEY_Y && action == GLFW_PRESS)
	{
		rotateX = false;
		rotateY = true;
		rotateZ = false;
	}

	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
	{
		rotateX = false;
		rotateY = false;
		rotateZ = true;
	}

	if ((key == GLFW_KEY_W || key == GLFW_KEY_UP) && action == GLFW_PRESS)
	{
		objects[selectedObject].offsetY += 0.5f;
	}
	if ((key == GLFW_KEY_S || key == GLFW_KEY_DOWN) && action == GLFW_PRESS)
	{
		objects[selectedObject].offsetY -= 0.5f;
	}
	if ((key == GLFW_KEY_A || key == GLFW_KEY_LEFT) && action == GLFW_PRESS)
	{
		objects[selectedObject].offsetX -= 0.5f;
	}
	if ((key == GLFW_KEY_D || key == GLFW_KEY_RIGHT) && action == GLFW_PRESS)
	{
		objects[selectedObject].offsetX += 0.5f;
	}
	if ((key == GLFW_KEY_Q || key == GLFW_KEY_LEFT) && action == GLFW_PRESS)
	{
		objects[selectedObject].offsetZ -= 0.5f;
	}
	if ((key == GLFW_KEY_E || key == GLFW_KEY_RIGHT) && action == GLFW_PRESS)
	{
		objects[selectedObject].offsetZ += 0.5f;
	}

	if ((key == GLFW_KEY_F || key == GLFW_KEY_LEFT) && action == GLFW_PRESS)
	{
		objects[selectedObject].scale -= 0.25f;
	}
	if ((key == GLFW_KEY_G || key == GLFW_KEY_RIGHT) && action == GLFW_PRESS)
	{
		objects[selectedObject].scale += 0.25f;
	}

	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
	{
		selectedObject = 0;
	}
	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
	{
		selectedObject = 1;
	}
}

//Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
// shader simples e único neste exemplo de código
// O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
// fragmentShader source no iniçio deste arquivo
// A função retorna o identificador do programa de shader
int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

int loadSimpleOBJ(string filePath, int &nVertices, vec3 color)
{
	vector <vec3> vertices;
	vector <vec2> texCoords;
	vector <vec3> normals;
	vector <GLfloat> vBuffer;

	ifstream arqEntrada;

	arqEntrada.open(filePath.c_str());
	if (arqEntrada.is_open())
	{
		//Fazer o parsing
		string line;
		while (!arqEntrada.eof())
		{
			getline(arqEntrada,line);
			istringstream ssline(line);
			string word;
			ssline >> word;
			if (word == "v")
			{
				vec3 vertice;
				ssline >> vertice.x >> vertice.y >> vertice.z;
				//cout << vertice.x << " " << vertice.y << " " << vertice.z << endl;
				vertices.push_back(vertice);

			}
			if (word == "vt")
			{
				vec2 vt;
				ssline >> vt.s >> vt.t;
				//cout << vertice.x << " " << vertice.y << " " << vertice.z << endl;
				texCoords.push_back(vt);

			}
			if (word == "vn")
			{
				vec3 normal;
				ssline >> normal.x >> normal.y >> normal.z;
				//cout << vertice.x << " " << vertice.y << " " << vertice.z << endl;
				normals.push_back(normal);

			}
			else if (word == "f")
			{
				while (ssline >> word) 
				{
					int vi, ti, ni;
					istringstream ss(word);
    				std::string index;

    				// Pega o índice do vértice
    				std::getline(ss, index, '/');
    				vi = std::stoi(index) - 1;  // Ajusta para índice 0

    				// Pega o índice da coordenada de textura
    				std::getline(ss, index, '/');
    				ti = std::stoi(index) - 1;

    				// Pega o índice da normal
    				std::getline(ss, index);
    				ni = std::stoi(index) - 1;

					//Recuperando os vértices do indice lido
					vBuffer.push_back(vertices[vi].x);
					vBuffer.push_back(vertices[vi].y);
					vBuffer.push_back(vertices[vi].z);
					
					//Atributo cor
					vBuffer.push_back(color.r);
					vBuffer.push_back(color.g);
					vBuffer.push_back(color.b);
					
					//Atributo cor
					vBuffer.push_back(normals[ni].x);
					vBuffer.push_back(normals[ni].y);
					vBuffer.push_back(normals[ni].z);
        			
					vBuffer.push_back(texCoords[ti].s);
					vBuffer.push_back(texCoords[ti].t);

        			// Exibindo os índices para verificação
       				// std::cout << "v: " << vi << ", vt: " << ti << ", vn: " << ni << std::endl;
    			}
			}
		}

		arqEntrada.close();

		cout << "Gerando o buffer de geometria..." << endl;
		GLuint VBO, VAO;

		//Geração do identificador do VBO
		glGenBuffers(1, &VBO);

		//Faz a conexão (vincula) do buffer como um buffer de array
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		//Envia os dados do array de floats para o buffer da OpenGl
		glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

		//Geração do identificador do VAO (Vertex Array Object)
		glGenVertexArrays(1, &VAO);

		// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
		// e os ponteiros para os atributos 
		glBindVertexArray(VAO);
		
		//Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando: 
		// Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
		// Numero de valores que o atributo tem (por ex, 3 coordenadas xyz) 
		// Tipo do dado
		// Se está normalizado (entre zero e um)
		// Tamanho em bytes 
		// Deslocamento a partir do byte zero 
		
    	// Layout da posição (location 0)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(0));
		glEnableVertexAttribArray(0);

		// Layout da cor (location 1)
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);

		// Layout da normal (location 2)
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
		glEnableVertexAttribArray(2);

		// Layout da UV (location 3)
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(9 * sizeof(GLfloat)));
		glEnableVertexAttribArray(3);

		// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
		glBindVertexArray(0);

		nVertices = vBuffer.size() / 11; // Cada vértice agora tem 11 floats!
		return VAO;
	}
	else
	{
		cout << "Erro ao tentar ler o arquivo " << filePath << endl;
		return -1;
	}
}

GLuint loadTexture(string filePath, int &width, int &height)
{
	GLuint texID; // id da textura a ser carregada

	// Gera o identificador da textura na memória
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	// Ajuste dos parâmetros de wrapping e filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Carregamento da imagem usando a função stbi_load da biblioteca stb_image
	int nrChannels;

	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3) // jpg, bmp
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else // assume que é 4 canais png
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture " << filePath << std::endl;
	}

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}
