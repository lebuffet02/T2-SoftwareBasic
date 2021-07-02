#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Para usar strings

#ifdef WIN32
#include <windows.h> // Apenas para Windows
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>   // Funções da OpenGL
#include <GL/glu.h>  // Funções da GLU
#include <GL/glut.h> // Funções da FreeGLUT
#endif

// SOIL é a biblioteca para leitura das imagens
#include <SOIL.h>

// Um pixel RGB (24 bits)
typedef struct
{
    unsigned char r, g, b;
} RGB8;

// Uma imagem RGB
typedef struct
{
    int width, height;
    RGB8 *img;
} Img;

// Protótipos
void load(char *name, Img *pic);
void uploadTexture();
void seamcarve(int targetWidth); // executa o algoritmo
void freemem();                  // limpa memória (caso tenha alocado dinamicamente)

void energia(long *energias, int larguraConsiderada);
void energiaAcumulada(long *energias, long *energiasAcumuladas, int larguraConsiderada);
void removeColuna(long *energias, long *energiasAcumuladas, int larguraConsiderada, Img *maskCopy);
void calculaMask(long *energias);

// Funções da interface gráfica e OpenGL
void init();
void draw();
void keyboard(unsigned char key, int x, int y);
void arrow_keys(int a_keys, int x, int y);

// Largura e altura da janela
int width, height;

// Largura desejada (selecionável)
int targetW;

// Identificadores de textura
GLuint tex[3];

// As 3 imagens
// pic[0] é o *source, pic[1] é o *mask e pic[2] é o *target
Img pic[3];
Img *source;
Img *mask;
Img *target;

// Ponteiro para energias
long *energias;

// Imagem selecionada (0,1,2)
int sel;

// Carrega uma imagem para a struct Img
void load(char *name, Img *pic)
{
    int chan;
    pic->img = (RGB8 *)SOIL_load_image(name, &pic->width, &pic->height, &chan, SOIL_LOAD_RGB);
    if (!pic->img)
    {
        printf("SOIL loading error: '%s'\n", SOIL_last_result());
        exit(1);
    }
    printf("Load: %d x %d x %d\n", pic->width, pic->height, chan);
}

void energiaAcumulada(long *energias, long *energiasAcumuladas, int larguraConsiderada)
{
    long(*ptrEnergias)
        [larguraConsiderada] = (long(*)[larguraConsiderada])energias;

    for (int x = 1; x < larguraConsiderada - 1; x++)
    {

        int coordX = x;
        int energiaAcumulada = ptrEnergias[0][coordX];

        for (int y = 1; y < target->height - 2; y++)
        {
            int coordXAux = coordX;
            long menorCusto = ptrEnergias[y + 1][coordX];

            if (coordX > 0 && ptrEnergias[y + 1][coordX - 1] < menorCusto)
            {
                menorCusto = ptrEnergias[y + 1][coordX - 1];
                coordXAux = coordX - 1;
            }

            if (coordX < larguraConsiderada - 1 && ptrEnergias[y + 1][coordX + 1] < menorCusto)
            {
                menorCusto = ptrEnergias[y + 1][coordX + 1];
                coordXAux = coordX + 1;
            }

            energiaAcumulada += menorCusto;
            coordX = coordXAux;
        }

        energiasAcumuladas[x] = energiaAcumulada;
    }
}

void energia(long *energias, int larguraConsiderada)
{
    RGB8(*ptr)
    [target->width] = (RGB8(*)[target->width])target->img;

    for (int y = 1; y < target->height - 1; y++)
    {
        int deltaX;
        int deltaY;

        boolean isBordaSuperior = y == 0;
        boolean isBordaInferior = y == target->height - 1;

        for (int x = 1; x < larguraConsiderada - 1; x++)
        {
            boolean isBordaEsquerda = x == 0;
            boolean isBordaDireita = x == larguraConsiderada - 1;

            if (isBordaEsquerda)
            {
                //calcula borda esquerda
                int deltaRx = ptr[y][x + 1].r - ptr[y][x + 2].r;
                int deltaGx = ptr[y][x + 1].g - ptr[y][x + 2].g;
                int deltaBx = ptr[y][x + 1].b - ptr[y][x + 2].b;
                deltaX = pow(deltaRx, 2) + pow(deltaGx, 2) + pow(deltaBx, 2);
            }
            else if (isBordaDireita)
            {
                //calcula borda direita
                int deltaRx = ptr[y][x - 1].r - ptr[y][x - 2].r;
                int deltaGx = ptr[y][x - 1].g - ptr[y][x - 2].g;
                int deltaBx = ptr[y][x - 1].b - ptr[y][x - 2].b;
                deltaX = pow(deltaRx, 2) + pow(deltaGx, 2) + pow(deltaBx, 2);
            }
            else
            {
                int deltaRx = ptr[y][x + 1].r - ptr[y][x - 1].r;
                int deltaGx = ptr[y][x + 1].g - ptr[y][x - 1].g;
                int deltaBx = ptr[y][x + 1].b - ptr[y][x - 1].b;
                deltaX = pow(deltaRx, 2) + pow(deltaGx, 2) + pow(deltaBx, 2);
            }

            if (isBordaSuperior)
            {
                //calcula borda superior
                int deltaRy = ptr[y + 1][x].r - ptr[y + 2][x].r;
                int deltaGy = ptr[y + 1][x].g - ptr[y + 2][x].g;
                int deltaBy = ptr[y + 1][x].b - ptr[y + 2][x].b;
                deltaY = pow(deltaRy, 2) + pow(deltaGy, 2) + pow(deltaBy, 2);
            }
            else if (isBordaInferior)
            {
                //calcula borda inferior
                int deltaRy = ptr[y - 1][x].r - ptr[y - 2][x].r;
                int deltaGy = ptr[y - 1][x].g - ptr[y - 2][x].g;
                int deltaBy = ptr[y - 1][x].b - ptr[y - 2][x].b;
                deltaY = pow(deltaRy, 2) + pow(deltaGy, 2) + pow(deltaBy, 2);
            }
            else
            {
                int deltaRy = ptr[y - 1][x].r - ptr[y + 1][x].r;
                int deltaGy = ptr[y - 1][x].g - ptr[y + 1][x].g;
                int deltaBy = ptr[y - 1][x].b - ptr[y + 1][x].b;
                deltaY = pow(deltaRy, 2) + pow(deltaGy, 2) + pow(deltaBy, 2);
            }

            energias[y * larguraConsiderada + x] = deltaX + deltaY;
        }
    }
}

void removeColuna(long *energias, long *energiasAcumuladas, int larguraConsiderada, Img *maskCopy)
{
    int minIndex = 0;
    long(*ptrEnergias)
        [target->width] = (long(*)[target->width])energias;

    RGB8(*ptrPixels)
    [target->width] = (RGB8(*)[target->width])target->img;

    RGB8(*ptrMaskCopy)
    [maskCopy->width] = (RGB8(*)[maskCopy->width])maskCopy->img;

    for (int i = 1; i < larguraConsiderada; i++)
    {
        if (energiasAcumuladas[i] < energiasAcumuladas[minIndex])
        {
            minIndex = i;
        }
    }

    int coordX = minIndex;
    int energiaAcumulada = ptrEnergias[0][coordX];

    for (int y = 0; y < target->height; y++)
    {

        //remove pixel e move outros para a esquerda
        for (int x = coordX; x < larguraConsiderada; x++)
        {
            if (x + 1 < larguraConsiderada)
            {
                ptrPixels[y][x] = ptrPixels[y][x + 1];
                //remove mask copiada
                ptrMaskCopy[y][x] = ptrMaskCopy[y][x + 1];
            }
        }

        //descobre a coordenada X do próximo pixel a ser removido

        if (y + 1 < target->height)
        {
            int coordXAux = coordX;
            long menorCusto = ptrEnergias[y + 1][coordX];

            if (coordX > 0 && ptrEnergias[y + 1][coordX - 1] < menorCusto)
            {
                menorCusto = ptrEnergias[y + 1][coordX - 1];
                coordXAux = coordX - 1;
            }

            if (coordX < larguraConsiderada - 1 && ptrEnergias[y + 1][coordX + 1] < menorCusto)
            {
                menorCusto = ptrEnergias[y + 1][coordX + 1];
                coordXAux = coordX + 1;
            }

            coordX = coordXAux;
        }
    }
}

void calculaMask (long *energias) {

        long(*ptrEnergias)
            [target->width] = (long(*)[target->width])energias;

        RGB8(*ptrPixels)
        [mask->width] = (RGB8(*)[mask->width])mask->img;
        
        for(int y = 0; y < mask->height; y++) {

            for(int x = 0; x < mask->width; x++) {

                if(ptrPixels[y][x].r > 200 && ptrPixels[y][x].g < 50 && ptrPixels[y][x].b < 50) {

                    ptrEnergias[y][x] = 0;
                }
                else if(ptrPixels[y][x].r < 50 && ptrPixels[y][x].g > 200 && ptrPixels[y][x].b < 50) {

                    ptrEnergias[y][x] = (signed long) 999999;
                } 
        }
    }
}

void seamcarve(int targetWidth)
{
    // Aplica o algoritmo e gera a saida em target->img...

    target->height = source->height;
    target->width = source->width;

    Img maskCopyStruct;
    RGB8 img[mask->height * mask->width];
    maskCopyStruct = (Img) {.height = mask->height, .width = mask->width, .img = img};

    Img *maskCopy = &maskCopyStruct;

    for (int i = 0; i < source->width * source->height; i++)
    {
        target->img[i] = source->img[i];
        maskCopy->img[i] = mask->img[i];
    }

    int diff = abs(target->width - targetWidth);

    for (int i = 0; i < diff; i++)
    {
        int larguraConsiderada = target->width - i;
        long energias[target->height * target->width];

        for (int j = 0; j < target->height * target->width; j++)
        {
            energias[j] = 0;
        }

        energia(energias, larguraConsiderada);

        long energiasAcumuladas[target->width];

        for (int j = 0; j < larguraConsiderada; j++)
        {
            energiasAcumuladas[j] = 0;
        }

        calculaMask(energias);
        energiaAcumulada(energias, energiasAcumuladas, larguraConsiderada);
        removeColuna(energias, energiasAcumuladas, larguraConsiderada, maskCopy);
    }

    RGB8(*ptr)
    [target->width] = (RGB8(*)[target->width])target->img;

    for (int y = 0; y < target->height; y++)
    {
        // for (int x = 0; x < targetWidth; x++)
        // ptr[y][x].r = ptr[y][x].g = 255;
        for (int x = targetWidth; x < target->width; x++)
            ptr[y][x].r = ptr[y][x].g = 0;
    }
    // Chame uploadTexture a cada vez que mudar
    // a imagem (pic[2])
    uploadTexture();
    glutPostRedisplay();
}

void freemem()
{
    // Libera a memória ocupada pelas 3 imagens
    free(pic[0].img);
    free(pic[1].img);
    free(pic[2].img);
}

/********************************************************************
 * 
 *  VOCÊ NÃO DEVE ALTERAR NADA NO PROGRAMA A PARTIR DESTE PONTO!
 *
 ********************************************************************/
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("seamcarving [origem] [mascara]\n");
        printf("Origem é a imagem original, mascara é a máscara desejada\n");
        exit(1);
    }
    glutInit(&argc, argv);

    // Define do modo de operacao da GLUT
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    // pic[0] -> imagem original
    // pic[1] -> máscara desejada
    // pic[2] -> resultado do algoritmo

    // Carrega as duas imagens
    load(argv[1], &pic[0]);
    load(argv[2], &pic[1]);

    if (pic[0].width != pic[1].width || pic[0].height != pic[1].height)
    {
        printf("Imagem e máscara com dimensões diferentes!\n");
        exit(1);
    }

    // A largura e altura da janela são calculadas de acordo com a maior
    // dimensão de cada imagem
    width = pic[0].width;
    height = pic[0].height;

    // A largura e altura da imagem de saída são iguais às da imagem original (1)
    pic[2].width = pic[1].width;
    pic[2].height = pic[1].height;

    // Ponteiros para as structs das imagens, para facilitar
    source = &pic[0];
    mask = &pic[1];
    target = &pic[2];

    // Largura desejada inicialmente é a largura da janela
    targetW = target->width;

    // Especifica o tamanho inicial em pixels da janela GLUT
    glutInitWindowSize(width, height);

    // Cria a janela passando como argumento o titulo da mesma
    glutCreateWindow("Seam Carving");

    // Registra a funcao callback de redesenho da janela de visualizacao
    glutDisplayFunc(draw);

    // Registra a funcao callback para tratamento das teclas ASCII
    glutKeyboardFunc(keyboard);

    // Registra a funcao callback para tratamento das setas
    glutSpecialFunc(arrow_keys);

    // Cria texturas em memória a partir dos pixels das imagens
    tex[0] = SOIL_create_OGL_texture((unsigned char *)pic[0].img, pic[0].width, pic[0].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);
    tex[1] = SOIL_create_OGL_texture((unsigned char *)pic[1].img, pic[1].width, pic[1].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

    // Exibe as dimensões na tela, para conferência
    printf("Origem  : %s %d x %d\n", argv[1], pic[0].width, pic[0].height);
    printf("Máscara : %s %d x %d\n", argv[2], pic[1].width, pic[0].height);
    sel = 0; // pic1

    // Define a janela de visualizacao 2D
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(0.0, width, height, 0.0);
    glMatrixMode(GL_MODELVIEW);

    // Aloca memória para a imagem de saída
    pic[2].img = malloc(pic[1].width * pic[1].height * 3); // W x H x 3 bytes (RGB)
    // Pinta a imagem resultante de preto!
    memset(pic[2].img, 0, width * height * 3);

    // Cria textura para a imagem de saída
    tex[2] = SOIL_create_OGL_texture((unsigned char *)pic[2].img, pic[2].width, pic[2].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

    // Entra no loop de eventos, não retorna
    glutMainLoop();
}

// Gerencia eventos de teclado
void keyboard(unsigned char key, int x, int y)
{
    if (key == 27)
    {
        // ESC: libera memória e finaliza
        freemem();
        exit(1);
    }
    if (key >= '1' && key <= '3')
        // 1-3: seleciona a imagem correspondente (origem, máscara e resultado)
        sel = key - '1';
    if (key == 's')
    {
        seamcarve(targetW);
    }
    glutPostRedisplay();
}

void arrow_keys(int a_keys, int x, int y)
{
    switch (a_keys)
    {
    case GLUT_KEY_RIGHT:
        if (targetW <= pic[2].width - 10)
            targetW += 10;
        seamcarve(targetW);
        break;
    case GLUT_KEY_LEFT:
        if (targetW > 10)
            targetW -= 10;
        seamcarve(targetW);
        break;
    default:
        break;
    }
}
// Faz upload da imagem para a textura,
// de forma a exibi-la na tela
void uploadTexture()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 target->width, target->height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, target->img);
    glDisable(GL_TEXTURE_2D);
}

// Callback de redesenho da tela
void draw()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Preto
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Para outras cores, veja exemplos em /etc/X11/rgb.txt

    glColor3ub(255, 255, 255); // branco

    // Ativa a textura corresponde à imagem desejada
    glBindTexture(GL_TEXTURE_2D, tex[sel]);
    // E desenha um retângulo que ocupa toda a tela
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);

    glTexCoord2f(0, 0);
    glVertex2f(0, 0);

    glTexCoord2f(1, 0);
    glVertex2f(pic[sel].width, 0);

    glTexCoord2f(1, 1);
    glVertex2f(pic[sel].width, pic[sel].height);

    glTexCoord2f(0, 1);
    glVertex2f(0, pic[sel].height);

    glEnd();
    glDisable(GL_TEXTURE_2D);

    // Exibe a imagem
    glutSwapBuffers();
}
