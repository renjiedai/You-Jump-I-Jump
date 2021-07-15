/* �ڲ�ȫ�����ռ�������ִ�У�����Ǻ��壬�������������ʱ����һ�����ĶԳƲ��� */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// board information
#define BOARD_SIZE 8
#define EMPTY 0
#define MY_FLAG 2
#define MY_KING 4
#define ENEMY_FLAG 1
#define ENEMY_KING 3

#define DEPTH 16
//�ڵ��������ʱ��洢�������
int searchDepth;
//���ڴ洢��ǰ�ǵڼ��غϣ���ֹ��󼸲�����̫�����60�غ���
int nowRound = -1;
#define MAX_ROUND 60


#define MAX_SCORE 2000000
#define MIN_SCORE -2000000
// bool
typedef int BOOL;
#define TRUE 1
#define FALSE 0
//���������һ���ƶ��Ĳ�������֪��Ϊʲô��15������ֻ��12�����ӣ������ƶ�����Ӧ�ò��ᳬ��13
#define MAX_STEP 15

#define START "START"
#define PLACE "PLACE"
#define TURN "TURN"
#define END "END"


#define TIME_FULL 1500

//����ṹ�壬��ߴ����·�ĺۼ�
struct Command
{
    int x[MAX_STEP];
    int y[MAX_STEP];
    int numStep;
};

//����һ��route���͵Ľṹ�壬�洢��һ�������õ�����õ��߷�
struct Route
{
    int numMove;
    struct Command step[DEPTH];
};

//����ȫ�ֱ��������̣���ʼ����ֵΪ0
char board[BOARD_SIZE][BOARD_SIZE] = { 0 };
//myFlag��ʾ�ҷ�ִ�ӵ���ɫ
int myFlag;
//��Ϊֻ��б���ߣ���ʾб���ߵķ���
int moveDir[4][2] = { {1, -1}, {1, 1}, {-1, -1}, {-1, 1} };

//��ʾ�����ߵķ�λ
int jumpDir[4][2] = { {2, -2}, {2, 2}, {-2, -2}, {-2, 2} };

//��ʾ�ҵ����������˵�����
int numMyFlag;
int numEnFlag;

struct Command moveCmd = { .x = {0}, .y = {0}, .numStep = 2 };
struct Command jumpCmd = { .x = {0}, .y = {0}, .numStep = 0 };
struct Command longestJumpCmd = { .x = {0}, .y = {0}, .numStep = 1 };

//����һ���ֿ⣬�洢���ܵ��߷������鿪��һЩ
struct Command tempMove[DEPTH + 1][24];
//���ڴ洢��һ�������ҵ��Ĵ���߷�
struct Route firstRoute;


//����һ������ר�����ڳ�ʼ���ķ���
struct Command init = { .x = {0}, .y = {0}, .numStep = 0 };
struct Command bestCmd = { .x = {0}, .y = {0}, .numStep = 0 };
int move = 0;

//unsigned long ���͵ģ����ڼ�¼ʱ��
DWORD startTime;
int timeUp = 0;

//���ڳ�ʼ��tempMove
void initMoveTry()
{
    for (int i = 0; i < DEPTH + 1; i++)
    {
        for (int j = 0; j < 24; j++)
        {
            tempMove[i][j] = init;
        }
    }
}

//����
void debug(const char* str)
{
    printf("DEBUG %s\n", str);
    fflush(stdout);
}
//Ū�����̣�����board���������̽��и�ֵ
void printBoard()
{
    char visualBoard[BOARD_SIZE][BOARD_SIZE + 1] = { 0 };
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            switch (board[i][j])
            {
            case EMPTY:
                visualBoard[i][j] = '.';
                break;
            case ENEMY_FLAG:
                visualBoard[i][j] = 'O';
                break;
            case MY_FLAG:
                visualBoard[i][j] = 'X';
                break;
            case ENEMY_KING:
                visualBoard[i][j] = '@';
                break;
            case MY_KING:
                visualBoard[i][j] = '*';
                break;
            default:
                break;
            }
        }
        printf("%s\n", visualBoard[i]);
    }
}
//�Ƿ���������
BOOL isInBound(int x, int y)
{
    return x >= 0 && x < BOARD_SIZE&& y >= 0 && y < BOARD_SIZE;
}

//����Լ��õ���1�����ͼ����ӣ����Է��ߵ�·���������ĶԳ�ʽ��ת
void rotateCommand(struct Command* cmd)
{
    if (myFlag == ENEMY_FLAG)
    {
        for (int i = 0; i < cmd->numStep; i++)
        {
            cmd->x[i] = BOARD_SIZE - 1 - cmd->x[i];
            cmd->y[i] = BOARD_SIZE - 1 - cmd->y[i];
        }
    }
}

/*
    ��x,y���괦������Ծ��currentStep�ǵ�ǰ������side�ǵ�ǰҪ���ӵ�һ����0Ϊ�ף�1Ϊ��
    ������û�н���ʵ���Ե��ƶ�
    �����ĸ�������һ�������ߡ���ô���ܵ���Զ������·����¼����
    ���ȼ�˳������ ���� ���� ����
*/
void tryToJump(int x, int y, int currentStep, int side, char board[BOARD_SIZE][BOARD_SIZE])
{
    //�״ν���ݹ��ʱ���ʼ��һ��
    if (currentStep == 0)
    {
        jumpCmd = init;
    }
    int newX, newY, midX, midY;
    char tmpFlag;
    jumpCmd.x[currentStep] = x;
    jumpCmd.y[currentStep] = y;
    //һ��ʼ����Ĭ����0���Ȱ���Ծ����++�����û�ҵ��������--
    jumpCmd.numStep++;
    for (int i = 0; i < 4; i++)
    {
        //���ȼ�˳������ ���� ���� ����
        newX = x + jumpDir[i][0];
        newY = y + jumpDir[i][1];
        midX = (x + newX) / 2;
        midY = (y + newY) / 2;
        //��������    ��ʼλ����һ������    �е��Ǹ���һ����Ҫ���Ե���    �µ����ǿյ�
        if (isInBound(newX, newY) && ((board[x][y] & 1) == side) && ((board[midX][midY] & 1) != side) && board[midX][midY] != EMPTY && (board[newX][newY] == EMPTY))
        {
            //������㵱���
            board[newX][newY] = board[x][y];
            //ԭ���Ǹ�����Ϊ��
            board[x][y] = EMPTY;
            //��Ҫ���Ե�������ʱ������
            tmpFlag = board[midX][midY];
            //���Ե��Ķ����ÿ�
            board[midX][midY] = EMPTY;
            //������㿴�����ܲ�����
            tryToJump(newX, newY, currentStep + 1, side, board);
            //�ָ���ԭ�������ӣ�ֻ��һ���������Ծ����
            board[x][y] = board[newX][newY];
            board[newX][newY] = EMPTY;
            board[midX][midY] = tmpFlag;
        }
    }
    if (jumpCmd.numStep > longestJumpCmd.numStep)
    {
        //��jump���ڴ��ַ������longest���ȥ
        memcpy(&longestJumpCmd, &jumpCmd, sizeof(struct Command));
    }
    jumpCmd.numStep--;
}

//����������Ӧ�ĵط����� �����ƶ� ����ȥ�� С�������Ȳ���
void place(char Mboard[BOARD_SIZE][BOARD_SIZE], struct Command cmd)
{
    int midX, midY, curFlag;
    //��������֮ǰ�Ǹ�λ�õĶ���
    curFlag = Mboard[cmd.x[0]][cmd.y[0]];
    //����������˳��
    for (int i = 0; i < cmd.numStep - 1; i++)
    {
        //�Ѿ��߹�ȥ�ĵط��ÿ�
        Mboard[cmd.x[i]][cmd.y[i]] = EMPTY;
        //ģ���ߵĹ��̣����ȥ
        Mboard[cmd.x[i + 1]][cmd.y[i + 1]] = curFlag;
        //����2��ʾ�������ߵ�
        if (abs(cmd.x[i] - cmd.x[i + 1]) == 2)
        {
            //���е����
            midX = (cmd.x[i] + cmd.x[i + 1]) / 2;
            midY = (cmd.y[i] + cmd.y[i + 1]) / 2;
            //�ǰ��ӣ��Ż�ִ��num����1������Ϊ���ҵ��ӱ�����
            if ((Mboard[midX][midY] & 1) == 0)
            {
                //�ҵ��Ӽ���һ�������Է�����
                //numMyFlag--;
            }
            //�м����Ǻ��ӣ��Է�����һ����������
            if ((Mboard[midX][midY] & 1) == 1)
            {
                //�ҵ��Ӽ���һ�������Է�����
                //numEnFlag--;
            }
            //��ǰλ���ÿգ���Ϊ�Ѿ�������
            Mboard[midX][midY] = EMPTY;
        }
    }
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        //�����ߵ��˰׵ĵײ�������Ϊ����
        if (Mboard[0][i] == ENEMY_FLAG)
        {
            Mboard[0][i] = ENEMY_KING;
        }
        //�����ߵ��˺ڵĵײ�������Ϊ����
        if (Mboard[BOARD_SIZE - 1][i] == MY_FLAG)
        {
            Mboard[BOARD_SIZE - 1][i] = MY_KING;
        }
    }
}

/**
 * YOUR CODE BEGIN
 * ��Ĵ��뿪ʼ
 */

 /**
  * You can define your own struct and variable here
  * ����������ﶨ�����Լ��Ľṹ��ͱ���
  */

  /**
   * ������������ʼ�����AI
   */
void initAI(int me)
{
    numMyFlag = 12;
    numEnFlag = 12;
}

//����ÿһ���巽�����ӣ����ж������ܷ��������������tempMove��Ӧ��depth��Ȳ㣬side=0�����ߣ�side=1�ǶԷ���
int createJump(char jumpBoard[BOARD_SIZE][BOARD_SIZE], int side, int depth)
{
    //�ѵ�ǰ��洢����Ķ����ÿ�
    /*for (int j = 0; j < 24; j++)
    {
        tempMove[depth][j] = init;
        if (tempMove[depth][j].x == 0 && tempMove[depth][j].y == 0)
        {
            break;
        }
    }*/

    struct Command command = { {0},{0}, 0 };
    int numChecked = 0;
    int maxStep = 1;
    //��¼�ҵ��������Ĵ���
    int jumps = 0;
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            if (jumpBoard[i][j] > 0 && (jumpBoard[i][j] & 1) == side)
            {
                //�ȶ�longest���г�ʼ������
                longestJumpCmd = init;
                numChecked++;
                longestJumpCmd.numStep = 1;
                //��ʼ��command
                command = init;
                //int tem1 = numEnemy, tem2 = numMe;
                //�������·���������longest
                tryToJump(i, j, 0, side, jumpBoard);
                //numEnemy = tem1; numMe = tem2;
                //�ҵ��˱�ԭ�������ģ����滻��ȥ
                if (longestJumpCmd.numStep > maxStep)
                {
                    //��һ��ʵ����û�бȽϽ��д�����ΪlongestJumpstep���ֱ��ˣ��ϱߵ�����������Ҳ��������̵���
                    maxStep = longestJumpCmd.numStep;
                    memcpy(&command, &longestJumpCmd, sizeof(struct Command));
                    //�ҵ��˱�ԭ�������ģ������ԭ���ģ����滻��ȥ
                    /*for (int j = 0; j < 24; j++)
                    {
                        tempMove[depth][j] = init;
                        if (tempMove[depth][j].x == 0 && tempMove[depth][j].y == 0)
                        {
                            break;
                        }
                    }*/
                    tempMove[depth][0] = command;
                    jumps = 1;
                }
                //�ҵ���һ�����ģ�Ҳ���Ƕ���һ������
                else if (longestJumpCmd.numStep == maxStep && longestJumpCmd.numStep != 1)
                {
                    //��һ��ʵ����û�бȽϽ��д�����ΪlongestJumpstep���ֱ��ˣ��ϱߵ�����������Ҳ��������̵���
                    //maxStep = longestJumpCmd.numStep;
                    memcpy(&command, &longestJumpCmd, sizeof(struct Command));
                    //�ҵ��˱�ԭ�������ģ����ں�ߵݼ�һ��
                    tempMove[depth][jumps] = command;
                    jumps++;
                }
            }
            if (side == 0 && numChecked == numMyFlag)
            {
                return jumps;
            }
            if (side == 1 && numChecked == numEnFlag)
            {
                return jumps;
            }
        }
    }
    return jumps;
}

//�����ƶ������ҽ����п��ƶ����߷��Ž�tempMove�ĵ�ǰ��Ȳ���
int tryToMove(int x, int y, int depth, int flag, char board[BOARD_SIZE][BOARD_SIZE])
{
    int newX, newY, trynum;
    int times = 0;
    //����Ƿ�Ϊ�����������ĸ�������
    if (board[x][y] == 4 || board[x][y] == 3)
    {
        trynum = 4;
    }
    else
    {
        trynum = 2;
    }
    for (int i = 0; i < trynum; i++)
    {
        if (flag == 0)
        {
            newX = x + moveDir[i][0];
            newY = y + moveDir[i][1];
        }
        else
        {
            newX = x - moveDir[i][0];
            newY = y - moveDir[i][1];
        }
        if (isInBound(newX, newY) && board[newX][newY] == EMPTY)
        {
            tempMove[depth][move].numStep = 2;
            tempMove[depth][move].x[0] = x;
            tempMove[depth][move].y[0] = y;
            tempMove[depth][move].x[1] = newX;
            tempMove[depth][move].y[1] = newY;
            times++;
            move++;
        }
    }
    return times;
}

//����ÿ�����巽���壬�ж������ܷ��ƶ�
int creatMove(char temboard[BOARD_SIZE][BOARD_SIZE], int depth, int flag)
{
    //�ѵ�ǰ��洢����Ķ����ÿ�
    /*for (int j = 0; j < 24; j++)
    {
        tempMove[depth][j] = init;
        if (tempMove[depth][j].x == 0 && tempMove[depth][j].y == 0)
        {
            break;
        }
    }*/

    int num = 0;
    int moves = 0;
    move = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            if (temboard[i][j] != EMPTY)
            {
                //�ֵ�������Ϊ���ӣ���������
                if (flag == 0 && ((temboard[i][j] & 1) == 1))
                {
                    continue;
                }
                //��������Ϊ���ӣ���������
                if (flag == 1 && ((temboard[i][j] & 1) == 0))
                {
                    continue;
                }
                num++;
                moves += tryToMove(i, j, depth, flag, temboard);
            }
        }
        if (flag == 0 && num == numMyFlag)
        {
            return moves;
        }
        if (flag == 1 && num == numEnFlag)
        {
            return moves;
        }
    }
    return moves;
}

//��ֵ������һ���֣�����������Ŀ��������
int evaluateChessNum(char board[BOARD_SIZE][BOARD_SIZE])
{
    int score = 0;
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            switch (board[i][j])
            {
            case ENEMY_FLAG: score -= 270; break;
            case MY_FLAG: score += 332; break;
            case ENEMY_KING: score -= 125; break;
            case MY_KING: score += 520; break;
            default: break;
            }
        }
    }

    return score;
}
//��ֵ�����ڶ����֣���������λ�ù�ϵ�������
int evaluateChessPosition(char board[BOARD_SIZE][BOARD_SIZE])
{
    int score = 0;
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            //����վ�мӳ�
            if (board[i][j] == MY_FLAG)
            {
                if (i == 0 || j == 7 || j == 0)
                {
                    score += 30;
                }
                continue;
            }
            if (board[i][j] == MY_KING)
            {
                if (i == 7 || j == 7)
                {
                    score += 75;
                }
                continue;
            }
            if (board[i][j] == ENEMY_FLAG)
            {
                if (i == 7 || j == 7 || j == 0)
                {
                    score -= 30;
                }
                continue;
            }
            if (board[i][j] == ENEMY_KING)
            {
                if (j == 7 || j == 0)
                {
                    score -= 75;
                }
                continue;
            }
        }
    }
    return score;
}
//��ֵ�����ڶ����֣����������Ƿ񱻹����������
int evaluateChessAttack(char board[BOARD_SIZE][BOARD_SIZE])
{
    int score = 0;
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            //�ҷ����ӣ��۲��ǲ��ǻᱻ�Է�����
            if (board[i][j] % 2 == 0)
            {
                for (int k = 0; k < 4; k++)
                {
                    int x = i + moveDir[k][0];
                    int y = j + moveDir[k][1];
                    int enx = i - moveDir[k][0];
                    int eny = j - moveDir[k][1];
                    if (isInBound(x, y) && isInBound(enx, eny) && ((board[x][y] == ENEMY_FLAG) ||
                        (board[x][y] == ENEMY_KING)) && (board[enx][eny] == EMPTY))
                    {
                        score -= 158;
                        break;
                    }
                }
                continue;
            }
            //�Է����ӣ��۲��ǲ����ܱ��ҷ����ӹ���
            if (board[i][j] % 2 == 1)
            {
                for (int k = 0; k < 4; k++)
                {
                    int x = i + moveDir[k][0];
                    int y = j + moveDir[k][1];
                    int enx = i - moveDir[k][0];
                    int eny = j - moveDir[k][1];
                    if (isInBound(x, y) && isInBound(enx, eny) && ((board[x][y] == MY_FLAG) ||
                        (board[x][y] == MY_KING)) && (board[enx][eny] == EMPTY))
                    {
                        score += 152;
                        break;
                    }
                }
                continue;
            }
        }
    }
    return score;
}


//�ܵĹ�ֵ����X
int evaluateX(char board[BOARD_SIZE][BOARD_SIZE])
{
    DWORD curTime;
    int val = 240;
    val += evaluateChessNum(board);
    val += evaluateChessPosition(board);
    val += evaluateChessAttack(board);

    curTime = GetTickCount();
    if (curTime - startTime > TIME_FULL)
    {
        timeUp = 1;
    }

    return val;
}
  
//���ڰѴ�������ŵ���һλ����һ�������ŵ�ǰ�ѵ���������ָ��
void sortStep(struct Command* pLine, int depth, int counts)
{
    int same = 0;
    struct Command temp;
    for (int i = 0; i < counts; i++)
    {
        //��ͬһ�����͵ģ�����
        if ((pLine + i)->numStep != firstRoute.step[searchDepth - depth].numStep)
        {
            continue;
        }

        //ͬһ�����͵�       
        same = 1;
        //�Һʹ����ȫһ����
        for (int j = 0; j < (pLine + i)->numStep; j++)
        {
            //�ʹ�粻һ�����ⶫ�����У�������
            if ((pLine + i)->x[j] != firstRoute.step[searchDepth - depth].x[j] || (pLine + i)->y[j] != firstRoute.step[searchDepth - depth].y[j])
            {
                same = 0;
                break;
            }
        }
        //�ҵ��˴��
        if (same == 1)
        {
            //�����ŵ���λ����������
            temp = *pLine;
            *pLine = *(pLine + i);
            *(pLine + i) = temp;
            break;
        }       
    }
}

////�����С������Alpha-Beta��֦
//int minmax(int curstep, int player, int alpha, int beta, struct Route* pRoute, int maxDepth)
//{
//    struct Route route;
//    //�洢������߷�
//    struct Command stepFind[24];
//    ////�ѵ�ǰ��洢����Ķ����ÿ�
//    //for (int j = 0; j < 24; j++)
//    //{
//    //    tempMove[maxDepth - curstep][j] = init;
//    //}
//
//    //����Ҷ�ӽڵ�
//    if (curstep == maxDepth)
//    {
//        //�ò��߷�Ϊ0�������ع�ֵ����
//        pRoute->numMove = 0;
//        return evaluateX(board);
//    }
//    if (numMyFlag == 0) return -2147483646 + 1;
//    if (numEnFlag == 0) return 2147483646 - 1;
//    char tempboard[BOARD_SIZE][BOARD_SIZE];
//
//    int count = 0;
//    int Talpha = -2147483646;
//    int Tbeta = 2147483646;
//
//    memcpy(&tempboard, &board, sizeof(board));
//    int nummyflag = numMyFlag;
//    int numeflag = numEnFlag;
//    //alpha-beta ��֦
//    if (player == MY_FLAG)
//    {
//        //����
//        /* BUG������ʱ��ֻ����һ����û�Ƚϣ���countû����ֵ */
//        count = createJump(board, curstep % 2, maxDepth - curstep);
//        //û�ҵ��������������move�ķ���
//        if (count == 0)
//        {
//            count = creatMove(board, maxDepth - curstep, curstep % 2);
//        }
//        //ת��������������
//        for (int j = 0; j < 24; j++)
//        {
//            stepFind[j] = tempMove[maxDepth - curstep][j];
//            /*if (stepFind[j].x == 0 && stepFind[j].y == 0)
//            {
//                break;
//            }*/
//        }
//
//        //��ʼ��ֻ����һ������ֱ���߾����ˣ�����������
//        if (curstep == 0 && count == 1)
//        {
//            bestCmd = stepFind[0];
//            return 520;
//        }
//        ////�Ҳ��ܶ��ˣ�ֱ���и�
//        //if (count == 0)
//        //{
//        //    return -2147483646 + 1;
//        //}
//
//        //ͨ����������������Ҫ�������Ѵ��ŵ�һ��λ�����Ѵ��
//        sortStep(stepFind, maxDepth - curstep, count);
//
//        Tbeta = beta;
//        for (int i = 0; i < count; i++)
//        {
//            place(stepFind[i]);
//            int a = minmax(curstep + 1, ENEMY_FLAG, Talpha, Tbeta, &route, maxDepth);
//            if (a > Talpha)
//            {
//                Talpha = a;
//                //û���ü���ȥ��ɴ���߷�
//                if (Talpha < Tbeta)
//                {
//                    //��������߷�
//                    pRoute->step[0] = stepFind[i];
//                    //�Ѵ�step��ʼ��num�������������һλ
//                    memcpy(pRoute->step + 1, route.step, route.numMove * sizeof(struct Command));
//                    pRoute->numMove = route.numMove + 1;
//                }
//                //����ݹ�����˾Ͱ���������
//                if (curstep == 0)
//                {
//                    bestCmd = stepFind[i];
//                }
//            }
//            if (Talpha >= Tbeta)
//            {
//                //��ʼ����break
//                memcpy(&board, &tempboard, sizeof(tempboard));
//                numMyFlag = nummyflag;
//                numEnFlag = numeflag;
//                break;
//            }
//            memcpy(&board, &tempboard, sizeof(tempboard));
//            numEnFlag = numeflag;
//            numMyFlag = nummyflag;
//        }
//        return Talpha;
//    }
//    else
//    {
//        count = createJump(board, curstep % 2, maxDepth - curstep);
//        //û�ҵ��������������move�ķ���
//        if (count == 0)
//        {
//            count = creatMove(board, maxDepth - curstep, curstep % 2);
//        }
//        //ת��������������
//        for (int j = 0; j < 24; j++)
//        {
//            stepFind[j] = tempMove[maxDepth - curstep][j];
//            if (stepFind[j].x == 0 && stepFind[j].y == 0)
//            {
//                break;
//            }
//        }
//
//
//        sortStep(stepFind, maxDepth - curstep, count);
//
//        Talpha = alpha;
//        for (int i = 0; i < count; i++)
//        {
//            //���ֵ����̲���
//            place(stepFind[i]);
//            int b = minmax(curstep + 1, MY_FLAG, Talpha, Tbeta, &route, maxDepth);
//            if (b < Tbeta)
//            {
//                Tbeta = b;
//                //û���ü���ȥ��ɴ���߷�
//                if (Talpha < Tbeta)
//                {
//                    //��������߷�
//                    pRoute->step[0] = stepFind[i];
//                    //�Ѵ�step��ʼ��num�������������һλ
//                    memcpy(pRoute->step + 1, route.step, route.numMove * sizeof(struct Command));
//                    pRoute->numMove = route.numMove + 1;
//                }
//            }
//            if (Talpha >= Tbeta)
//            {
//                memcpy(&board, &tempboard, sizeof(tempboard));
//                numMyFlag = nummyflag;
//                numEnFlag = numeflag;
//                break;
//            }
//            memcpy(&board, &tempboard, sizeof(tempboard));
//            numEnFlag = numeflag;
//            numMyFlag = nummyflag;
//        }
//        return Tbeta;
//    }
//}

/*
    ���nowFlag��myflag����enemyflag
    nowFlag��1����2��ʾ�Է����ҷ�
*/
int negaMax(int depth, int alpha, int beta, const char boardLast[BOARD_SIZE][BOARD_SIZE], struct Route* pRoute, int nowFlag)
{
    struct Route route;
    struct Command stepFind[24];
    int score;
    char boardNext[BOARD_SIZE][BOARD_SIZE];
    int count = 0;
    memcpy(boardNext, boardLast, sizeof(boardNext));

    //����Ҷ�ӽڵ㣬���ù�ֵ����
    if (depth == 0)
    {
        //����ֵʵ������������
        pRoute->numMove = 0;
        return evaluateX(boardNext);
    }

    //����
        /* BUG������ʱ��ֻ����һ����û�Ƚϣ���countû����ֵ */
    count = createJump(boardNext, nowFlag % 2, depth);
    //û�ҵ��������������move�ķ���
    if (count == 0)
    {
        count = creatMove(boardNext, depth, nowFlag % 2);
    }
    //ת��������������
    for (int j = 0; j < 24; j++)
    {
        stepFind[j] = tempMove[depth][j];
    }

    //������
    if (count != 0)
    {

        //����ȥ�Ҵ���߷�����Ҫ�����Ż���֦�Ĳü�Ч��
        sortStep(stepFind, depth, count);

        //�������е�ȥ����һ��
        for (int i = 0; i < count; i++)
        {
            memcpy(boardNext, boardLast, sizeof(boardNext));
            place(boardNext, stepFind[i]);
            score = -negaMax(depth - 1, -beta, -alpha, (const char(*)[BOARD_SIZE])boardNext, &route, 3 - nowFlag);

            //ʱ�䵽�˾ͷ���
            if (timeUp == 1)
            {
                return 0;
            }

            if (score >= beta)
            {
                return beta;
            }
            if (score > alpha)
            {
                alpha = score;

                //Generate the firstRoute
                pRoute->step[0] = stepFind[i];
                //�Ѵ�step��ʼ��num�������������һλ
                memcpy(pRoute->step + 1, route.step, route.numMove * sizeof(struct Command));
                pRoute->numMove = route.numMove + 1;
            }
        }
        return alpha;
    }

    //�������ˣ��������ˣ�ֱ�ӷ���
    else
    {
        return -214748364;
    }
}

/**
 * �ֵ������ӡ�
 * ������0��ʾ�հף�1��ʾ���壬2��ʾ����
 * me��ʾ�������������(1��2)
 * ����Ҫ����һ���ṹ��Command������numStep����Ҫ�ƶ������Ӿ����ĸ�����������㡢�յ㣩��
 * x��y�ֱ��Ǹ��������ξ�����ÿ�����ӵĺᡢ������
 * ������������
 */
struct Command aiTurn(const char board[BOARD_SIZE][BOARD_SIZE], int me)
{  
    initMoveTry();
    struct Route tempFirstRoute;
    //һ��һ�㣬��������
    for (searchDepth = 2; (searchDepth <= DEPTH && searchDepth <= 2 * (MAX_ROUND - nowRound)); searchDepth += 2)
    {
        //��tempFirstRoute����
        memset(&tempFirstRoute, 0, sizeof(struct Route));
        negaMax(searchDepth, -2147483647, 2147483647, board, &tempFirstRoute, MY_FLAG);
        //ʱ�䵽������ֹͣ
        if (timeUp == 1)
        {
            break;
        }
        //��temp�ĸ����ȫ�ֱ������ȥ
        memcpy(&firstRoute, &tempFirstRoute, sizeof(struct Route));     
    }
    //ʱ������Ϊ��
    timeUp = 0;

    return firstRoute.step[0];
}

/**
 * ��Ĵ������
 */

 //.X.X.X.X
 //X.X.X.X.
 //.X.X.X.X
 //........
 //........
 //O.O.O.O.
 //.O.O.O.O
 //O.O.O.O.
void start(int flag)
{
    //��ȫ����ʼ��Ϊ0
    memset(board, 0, sizeof(board));
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 8; j += 2)
        {
            //ʵ�ֽ�����ǵ�Ч�����ϱ����Ž�����ϲ���(2)
            board[i][j + (i + 1) % 2] = MY_FLAG;
        }
    }
    for (int i = 5; i < 8; i++)
    {
        for (int j = 0; j < 8; j += 2)
        {
            //ͬ�������Ŵ���ԲȦ(1)
            board[i][j + (i + 1) % 2] = ENEMY_FLAG;
        }
    }
    //���Լ���AI����һЩ������Ϊ����demo�������˸��Լ���ֵ��12
    initAI(flag);
}

void turn()
{
    //��ǰ�غ�����һ
    nowRound++;
    startTime = GetTickCount();
    // AI
    //���command��߼�¼��aiTurn���ҵ��ģ�Ҫ�ߵ�·��
    struct Command command = aiTurn((const char(*)[BOARD_SIZE])board, myFlag);
    place(board, command);
    //����Լ��Ǻ��ӣ���ת��Ȼ�����߷���
    rotateCommand(&command);
    //����������
    printf("%d", command.numStep);
    for (int i = 0; i < command.numStep; i++)
    {
        printf(" %d,%d", command.x[i], command.y[i]);
    }
    printf("\n");
    fflush(stdout);
}
//��������
void end(int x)
{
    exit(0);
}

void loop()
{
    //���ڽ���ϵͳ����ָ���������Ϸ����ǰ��ʼ������Ϸ�иò���ֻ����һ��
    char tag[10] = { 0 };
    //���command���Է����ӵ���Ϣ
    struct Command command =
    {
        .x = {0},
        .y = {0},
        .numStep = 0
    };
    int status;
    while (TRUE)
    {
        //�������С�ĵط�ȫ����ʼ����Ϊ0
        memset(tag, 0, sizeof(tag));
        scanf("%s", tag);
        //�յ��Ķ�����START��ʼ��־
        if (strcmp(tag, START) == 0)
        {
            //��myflag���붫��������������������ɫ
            scanf("%d", &myFlag);
            //����һЩ����ɶ�ĳ�ʼ������
            start(myFlag);
            //������ϵͳ�ظ�һ��OK��ʾ�յ�
            printf("OK\n");
            //ˢ�±�׼���
            fflush(stdout);
        }
        //�յ���ָ����PLACE����ʾ��Ҫ���ն��ֵ�����
        else if (strcmp(tag, PLACE) == 0)
        {
            //���նԷ��ƶ��Ĳ���
            scanf("%d", &command.numStep);
            //���ݶԷ��ƶ��Ĳ������ж��������ʼ�մ�0��ʼ����������
            for (int i = 0; i < command.numStep; i++)
            {
                scanf("%d,%d", &command.x[i], &command.y[i]);
            }
            //����Լ��Ǻڷ���ͨ����һ��ת��Ϊ��ɫ��������·��
            rotateCommand(&command);
            //��������
            place(board,command);
        }
        //�յ�TURN��ʾ�ֵ���������
        else if (strcmp(tag, TURN) == 0)
        {
            initMoveTry();
            //���м���������Ϊ
            turn();
        }
        //�յ�ENDָ���ʾ�������
        else if (strcmp(tag, END) == 0)
        {
            //��ȡ��ϵͳ���Ķ���������
            scanf("%d", &status);
            end(status);
        }
        printBoard();
    }
}

int main(int argc, char* argv[])
{
    loop();
    return 0;
}
