#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

float cameraX = 0.0f, cameraY = 1.0f, cameraZ = 3.0f;
float yaw = -90.0f;
float pitch = 0.0f;
float speed = 0.05f;

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraZ -= speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraZ += speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraX -= speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraX += speed;
}

int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(800, 600, "C++ FPS Demo", NULL, NULL);
    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBegin(GL_QUADS);
        glColor3f(0.3f, 0.8f, 0.3f);

        glVertex3f(-10.0f, 0.0f, -10.0f);
        glVertex3f( 10.0f, 0.0f, -10.0f);
        glVertex3f( 10.0f, 0.0f,  10.0f);
        glVertex3f(-10.0f, 0.0f,  10.0f);

        glEnd();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}