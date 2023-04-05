#include <cmath>
#include <iostream>
#include <thread>
#include <fstream>
#include <string>
#include "SFML/Graphics.hpp"

void draw_face(float *v0, float *v1, float *v2, sf::Image &image, float **zbuffer);

const float K2 = 3;//offsets z component of our object

unsigned int screen_width = 500;
unsigned int screen_height = 500;

float K1 = std::min(screen_width, screen_height)*1.2;//scaling factor

void x_prod(float *v0, float *v1, float *result)
{
    result[0] = v0[1] * v1[2] - v0[2] * v1[1];
    result[1] = v0[2] * v1[0] - v0[0] * v1[2];
    result[2] = v0[0] * v1[1] - v0[1] * v1[0];

    float length = sqrt(result[0] * result[0] + result[1] * result[1] + result[2] * result[2]);

    result[0] /= length;
    result[1] /= length;
    result[2] /= length;
}

inline float f_edge(float *v0, float *v1, float *v2)
{
    return (v2[0] - v0[0]) * (v1[1] - v0[1]) - (v2[1] - v0[1]) * (v1[0] - v0[0]);
}

void utah(float A, float B, sf::Image &image)
{
    float cosA = cos(A), sinA = sin(A);
    float cosB = cos(B), sinB = sin(B);

    //<load from file>
    std::ifstream fin("cup.off");
    std::string filetype;
    fin >> filetype;
    if(filetype != "OFF")
    {
        std::cout << "error" << std::endl;
        return;
    }
    int num_vert, num_faces;
    fin >> num_vert >> num_faces;

    //2 coordinates of x and y, next 1/z and finally normal vector associated with this vertex
    float **vertices = new float *[num_vert];

    for(int i = 0; i < num_vert; i++)
    {
        float x, y, z;
        vertices[i] = new float[6];//that is why 6 and not 3
        fin >> x >> y >> z;


        z -= 0.5;
        x -= 0.5;

        //here you have real coordinates that go to rotate
        vertices[i][0] = x * cosB + y * sinB;
        vertices[i][1] = -x * sinB * cosA + y * cosB * cosA + z * sinA;
        vertices[i][2] = x * sinA * sinB - y * sinA * cosB + z * cosA + K2;

        vertices[i][3] = vertices[i][4] = vertices[i][5] = 0;
    }

    //first is num of vertices, then these vertices
    int **faces = new int *[num_faces];
    for (int i = 0; i < num_faces; i++)
        faces[i] = new int[5];


    //calculation of normal vectors
    for(int i = 0; i < num_faces; i++)
    {
        fin >> faces[i][0];
        for (int j = 0; j < faces[i][0]; j++)
        {
            fin >> faces[i][j+1];
            if (faces[i][j+1] >= num_vert)
            {
                std::cout << faces[i][j+1] << std::endl;
                return;
            }
        }

        float face_norm[3];
        float v0[3] = {vertices[faces[i][2]][0] - vertices[faces[i][1]][0], vertices[faces[i][2]][1] - vertices[faces[i][1]][1], vertices[faces[i][2]][2] - vertices[faces[i][1]][2]};
        float v1[3] = {vertices[faces[i][3]][0] - vertices[faces[i][1]][0], vertices[faces[i][3]][1] - vertices[faces[i][1]][1], vertices[faces[i][3]][2] - vertices[faces[i][1]][2]};
        x_prod(v0, v1, face_norm);

        for(int j = 0; j < faces[i][0]; j++)
            for(int k = 0; k < 3; k++)
                vertices[faces[i][j+1]][k + 3] += face_norm[k];
    }

    for(int i = 0; i < num_vert; i++)
    {
        float length = sqrt(vertices[i][3] * vertices[i][3] + vertices[i][4] * vertices[i][4] + vertices[i][5] * vertices[i][5]);
        for (int j = 3; j < 6; j++)
            vertices[i][j] /= length;
            
        //transform to 2D coordinates
        vertices[i][2] = 1 / vertices[i][2];
        vertices[i][0] = screen_width / 2 + K1 * vertices[i][2] * vertices[i][0];
        vertices[i][1] = screen_height / 2 - K1 * vertices[i][2] * vertices[i][1];
    }

    //</load from file>

    float **zbuffer = new float *[screen_width];
    for(int i = 0; i < screen_width; i++)
    {
        zbuffer[i] = new float[screen_height];
        for (int j = 0; j < screen_height; j++)
            zbuffer[i][j] = 0;
    }


    for (int i = 0; i < num_faces; i++)
    {
        //there you should draw face instead of edges
        draw_face(vertices[faces[i][1]], vertices[faces[i][2]], vertices[faces[i][3]], image, zbuffer);
        if(faces[i][0] == 4)
            draw_face(vertices[faces[i][3]], vertices[faces[i][4]], vertices[faces[i][1]], image, zbuffer);
        //up to there
    }

    for(int i = 0; i < num_faces; i++)
        delete faces[i];
    delete faces;

    for(int i = 0; i < screen_width; i++)
        delete zbuffer[i];
    delete zbuffer;

    for (int i = 0; i < num_vert; i++)
        delete vertices[i];
    delete vertices;

    fin.close();
}

void draw_face(float *v0, float *v1, float *v2, sf::Image &image, float **zbuffer)
{
    float xmin, xmax, ymin, ymax;
    xmin = xmax = v0[0];
    ymin = ymax = v0[1];
    float *points[3] = {v0, v1, v2};

    //bounding box
    for (int i = 1; i < 3; i++)
    {
        if (points[i][0] < xmin)
            xmin = points[i][0];
        else if (points[i][0] > xmax)
            xmax = points[i][0];
        if(points[i][1] < ymin)
            ymin = points[i][1];
        else if(points[i][1] > ymax)
            ymax = points[i][1];
    }

    //rasterization
    float area = f_edge(v0, v1, v2);
    for(int x = xmin; x < xmax; x++)
        for (int y = ymin; y < ymax; y++)
            if(x >= 0 && y >= 0 && x < screen_width && y < screen_height)
            {
                float point[2] = {x, y};

                float w0 = f_edge(v0, v1, point) / area;
                float w1 = f_edge(v1, v2, point) / area;
                float w2 = f_edge(v2, v0, point) / area;

                if(w0 >= 0 && w1 >= 0 && w2 >= 0)
                {

                    float ooz = w0 * v2[2] + w1 * v0[2] + w2 * v1[2];
                    if(zbuffer[x][y] < ooz)
                    {
                        //here you calculate normal vector of this point
                        float normal[3];
                        
                        for(int i = 0; i < 3; i++)
                            normal[i] = (w0 * v2[i + 3] * v2[2] + w1 * v0[i + 3] * v0[2] + w2 * v1[i + 3] * v1[2]) / ooz;

                        float length = sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
                        for (int i = 0; i < 3; i++)
                            normal[i] /= length;

                        int intensity = (1 / sqrt(3) * normal[0] - 1 / sqrt(3) * normal[1] + 1 / sqrt(3) * normal[2]) * 255;

                        //temporary tweak
                        
                        if(intensity > 255)
                            intensity = 255;
                        else if(intensity < 0)
                            intensity = 0;

                        zbuffer[x][y] = ooz;
                        image.setPixel(x, y, sf::Color(intensity, intensity, intensity));
                    }
                }
            }
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(screen_width, screen_height), "Teapot");
    float B = 0;

    sf::Texture texture;
    texture.create(screen_width, screen_height);

    int count = 1;

    while(window.isOpen())
    {

        sf::Event event;
        while(window.pollEvent(event))
        {
            if(event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();

        sf::Image image;
        image.create(screen_width, screen_height, sf::Color(0, 0, 0));
        utah(M_PI/2+0.2, B, image);
        texture.update(image);
        sf::Sprite sprite(texture);
        window.draw(sprite);

        window.display();

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        B += M_PI / 100;
    }
    return 0;
}
