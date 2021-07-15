/* 内部全部按照己方白棋执行，如果是黑棋，仅仅在输入输出时进行一下中心对称操作 */
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
//在迭代加深的时候存储层数深度
int searchDepth;
//用于存储当前是第几回合，防止最后几步搜索太深超过了60回合数
int nowRound = -1;
#define MAX_ROUND 60


#define MAX_SCORE 2000000
#define MIN_SCORE -2000000
// bool
typedef int BOOL;
#define TRUE 1
#define FALSE 0
//对手最大能一次移动的步数，不知道为什么是15，对面只有12个棋子，吃子移动步数应该不会超过13
#define MAX_STEP 15

#define START "START"
#define PLACE "PLACE"
#define TURN "TURN"
#define END "END"


#define TIME_FULL 1500

//定义结构体，里边存放走路的痕迹
struct Command
{
    int x[MAX_STEP];
    int y[MAX_STEP];
    int numStep;
};

//定义一个route类型的结构体，存储上一次搜索得到的最好的走法
struct Route
{
    int numMove;
    struct Command step[DEPTH];
};

//定义全局变量当棋盘，初始化赋值为0
char board[BOARD_SIZE][BOARD_SIZE] = { 0 };
//myFlag表示我方执子的颜色
int myFlag;
//因为只能斜着走，表示斜着走的方向
int moveDir[4][2] = { {1, -1}, {1, 1}, {-1, -1}, {-1, 1} };

//表示跳着走的方位
int jumpDir[4][2] = { {2, -2}, {2, 2}, {-2, -2}, {-2, 2} };

//表示我的子数、敌人的子数
int numMyFlag;
int numEnFlag;

struct Command moveCmd = { .x = {0}, .y = {0}, .numStep = 2 };
struct Command jumpCmd = { .x = {0}, .y = {0}, .numStep = 0 };
struct Command longestJumpCmd = { .x = {0}, .y = {0}, .numStep = 1 };

//定义一个仓库，存储可能的走法，数组开大一些
struct Command tempMove[DEPTH + 1][24];
//用于存储上一次搜索找到的大哥走法
struct Route firstRoute;


//定义一个变量专门用于初始化的方便
struct Command init = { .x = {0}, .y = {0}, .numStep = 0 };
struct Command bestCmd = { .x = {0}, .y = {0}, .numStep = 0 };
int move = 0;

//unsigned long 类型的，用于记录时间
DWORD startTime;
int timeUp = 0;

//用于初始化tempMove
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

//调试
void debug(const char* str)
{
    printf("DEBUG %s\n", str);
    fflush(stdout);
}
//弄个棋盘，根据board对虚拟棋盘进行赋值
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
//是否在棋盘上
BOOL isInBound(int x, int y)
{
    return x >= 0 && x < BOARD_SIZE&& y >= 0 && y < BOARD_SIZE;
}

//如果自己拿的是1号类型即黑子，将对方走的路径进行中心对称式旋转
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
    从x,y坐标处尝试跳跃，currentStep是当前步数，side是当前要走子的一方，0为白，1为黑
    不过并没有进行实质性的移动
    遍历四个方向，找一下往哪走、怎么走跑的最远，并把路径记录下来
    优先级顺序：左下 右下 左上 右上
*/
void tryToJump(int x, int y, int currentStep, int side, char board[BOARD_SIZE][BOARD_SIZE])
{
    //首次进入递归的时候初始化一次
    if (currentStep == 0)
    {
        jumpCmd = init;
    }
    int newX, newY, midX, midY;
    char tmpFlag;
    jumpCmd.x[currentStep] = x;
    jumpCmd.y[currentStep] = y;
    //一开始进来默认是0，先把跳跃步数++，如果没找到，后边再--
    jumpCmd.numStep++;
    for (int i = 0; i < 4; i++)
    {
        //优先级顺序：左下 右下 左上 右上
        newX = x + jumpDir[i][0];
        newY = y + jumpDir[i][1];
        midX = (x + newX) / 2;
        midY = (y + newY) / 2;
        //在棋盘上    起始位置是一方的子    中点是个另一方、要被吃的子    新地盘是空的
        if (isInBound(newX, newY) && ((board[x][y] & 1) == side) && ((board[midX][midY] & 1) != side) && board[midX][midY] != EMPTY && (board[newX][newY] == EMPTY))
        {
            //以这个点当起点
            board[newX][newY] = board[x][y];
            //原来那个点置为空
            board[x][y] = EMPTY;
            //将要被吃掉的子暂时存起来
            tmpFlag = board[midX][midY];
            //被吃掉的东西置空
            board[midX][midY] = EMPTY;
            //在新起点看看还能不能跳
            tryToJump(newX, newY, currentStep + 1, side, board);
            //恢复成原来的样子，只是一个虚拟的跳跃尝试
            board[x][y] = board[newX][newY];
            board[newX][newY] = EMPTY;
            board[midX][midY] = tmpFlag;
        }
    }
    if (jumpCmd.numStep > longestJumpCmd.numStep)
    {
        //把jump的内存地址拷贝到longest里边去
        memcpy(&longestJumpCmd, &jumpCmd, sizeof(struct Command));
    }
    jumpCmd.numStep--;
}

//对棋盘上相应的地方进行 棋子移动 吃子去掉 小兵变王等操作
void place(char Mboard[BOARD_SIZE][BOARD_SIZE], struct Command cmd)
{
    int midX, midY, curFlag;
    //棋盘上走之前那个位置的东西
    curFlag = Mboard[cmd.x[0]][cmd.y[0]];
    //遍历的走子顺序
    for (int i = 0; i < cmd.numStep - 1; i++)
    {
        //已经走过去的地方置空
        Mboard[cmd.x[i]][cmd.y[i]] = EMPTY;
        //模拟走的过程，搬过去
        Mboard[cmd.x[i + 1]][cmd.y[i + 1]] = curFlag;
        //等于2表示是跳着走的
        if (abs(cmd.x[i] - cmd.x[i + 1]) == 2)
        {
            //找中点操作
            midX = (cmd.x[i] + cmd.x[i + 1]) / 2;
            midY = (cmd.y[i] + cmd.y[i + 1]) / 2;
            //是白子，才会执行num减少1个的行为，我的子被吃了
            if ((Mboard[midX][midY] & 1) == 0)
            {
                //我的子减少一个，被对方吃了
                //numMyFlag--;
            }
            //中间子是黑子，对方子少一个，被吃了
            if ((Mboard[midX][midY] & 1) == 1)
            {
                //我的子减少一个，被对方吃了
                //numEnFlag--;
            }
            //当前位置置空，因为已经被吃了
            Mboard[midX][midY] = EMPTY;
        }
    }
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        //黑子走到了白的底部，升级为黑王
        if (Mboard[0][i] == ENEMY_FLAG)
        {
            Mboard[0][i] = ENEMY_KING;
        }
        //白子走到了黑的底部，升级为白王
        if (Mboard[BOARD_SIZE - 1][i] == MY_FLAG)
        {
            Mboard[BOARD_SIZE - 1][i] = MY_KING;
        }
    }
}

/**
 * YOUR CODE BEGIN
 * 你的代码开始
 */

 /**
  * You can define your own struct and variable here
  * 你可以在这里定义你自己的结构体和变量
  */

  /**
   * 你可以在这里初始化你的AI
   */
void initAI(int me)
{
    numMyFlag = 12;
    numEnFlag = 12;
}

//遍历每一个棋方的棋子，并判断他们能否跳，能跳则存入tempMove对应的depth深度层，side=0是我走，side=1是对方走
int createJump(char jumpBoard[BOARD_SIZE][BOARD_SIZE], int side, int depth)
{
    //把当前层存储步骤的东西置空
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
    //记录找到的能跳的次数
    int jumps = 0;
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            if (jumpBoard[i][j] > 0 && (jumpBoard[i][j] & 1) == side)
            {
                //先对longest进行初始化操作
                longestJumpCmd = init;
                numChecked++;
                longestJumpCmd.numStep = 1;
                //初始化command
                command = init;
                //int tem1 = numEnemy, tem2 = numMe;
                //尝试找最长路径，最长存入longest
                tryToJump(i, j, 0, side, jumpBoard);
                //numEnemy = tem1; numMe = tem2;
                //找到了比原来更长的，就替换下去
                if (longestJumpCmd.numStep > maxStep)
                {
                    //这一步实际上没有比较进行处理，因为longestJumpstep数字变了，上边单点最长函数就找不到比它短的了
                    maxStep = longestJumpCmd.numStep;
                    memcpy(&command, &longestJumpCmd, sizeof(struct Command));
                    //找到了比原来更长的，先清空原来的，再替换下去
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
                //找到了一样长的，也就是多了一个方案
                else if (longestJumpCmd.numStep == maxStep && longestJumpCmd.numStep != 1)
                {
                    //这一步实际上没有比较进行处理，因为longestJumpstep数字变了，上边单点最长函数就找不到比它短的了
                    //maxStep = longestJumpCmd.numStep;
                    memcpy(&command, &longestJumpCmd, sizeof(struct Command));
                    //找到了比原来更长的，就在后边递加一次
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

//尝试移动，并且将所有可移动的走法放进tempMove的当前深度层中
int tryToMove(int x, int y, int depth, int flag, char board[BOARD_SIZE][BOARD_SIZE])
{
    int newX, newY, trynum;
    int times = 0;
    //检测是否为王，王可以四个方向走
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

//遍历每个下棋方的棋，判断他们能否移动
int creatMove(char temboard[BOARD_SIZE][BOARD_SIZE], int depth, int flag)
{
    //把当前层存储步骤的东西置空
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
                //轮到我走且为黑子，跳过继续
                if (flag == 0 && ((temboard[i][j] & 1) == 1))
                {
                    continue;
                }
                //对面走且为白子，跳过继续
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

//估值函数第一部分：根据棋子数目计算子力
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
//估值函数第二部分：根据棋子位置关系计算局面
int evaluateChessPosition(char board[BOARD_SIZE][BOARD_SIZE])
{
    int score = 0;
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            //靠边站有加成
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
//估值函数第二部分：根据棋子是否被攻击计算局面
int evaluateChessAttack(char board[BOARD_SIZE][BOARD_SIZE])
{
    int score = 0;
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            //我方棋子，观察是不是会被对方攻击
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
            //对方棋子，观察是不是能被我方棋子攻击
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


//总的估值函数X
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
  
//用于把大哥做法放到第一位，第一个参数放当前搜到的做法的指针
void sortStep(struct Command* pLine, int depth, int counts)
{
    int same = 0;
    struct Command temp;
    for (int i = 0; i < counts; i++)
    {
        //非同一种类型的，跳过
        if ((pLine + i)->numStep != firstRoute.step[searchDepth - depth].numStep)
        {
            continue;
        }

        //同一种类型的       
        same = 1;
        //找和大哥完全一样的
        for (int j = 0; j < (pLine + i)->numStep; j++)
        {
            //和大哥不一样，这东西不行，跳过它
            if ((pLine + i)->x[j] != firstRoute.step[searchDepth - depth].x[j] || (pLine + i)->y[j] != firstRoute.step[searchDepth - depth].y[j])
            {
                same = 0;
                break;
            }
        }
        //找到了大哥
        if (same == 1)
        {
            //把它放到首位，三步交换
            temp = *pLine;
            *pLine = *(pLine + i);
            *(pLine + i) = temp;
            break;
        }       
    }
}

////最大最小搜索和Alpha-Beta剪枝
//int minmax(int curstep, int player, int alpha, int beta, struct Route* pRoute, int maxDepth)
//{
//    struct Route route;
//    //存储本层的走法
//    struct Command stepFind[24];
//    ////把当前层存储步骤的东西置空
//    //for (int j = 0; j < 24; j++)
//    //{
//    //    tempMove[maxDepth - curstep][j] = init;
//    //}
//
//    //到达叶子节点
//    if (curstep == maxDepth)
//    {
//        //该层走法为0，并返回估值函数
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
//    //alpha-beta 剪枝
//    if (player == MY_FLAG)
//    {
//        //走子
//        /* BUG，跳的时候只能跳一个，没比较，且count没给赋值 */
//        count = createJump(board, curstep % 2, maxDepth - curstep);
//        //没找到能跳的情况，找move的法子
//        if (count == 0)
//        {
//            count = creatMove(board, maxDepth - curstep, curstep % 2);
//        }
//        //转换过来给存起来
//        for (int j = 0; j < 24; j++)
//        {
//            stepFind[j] = tempMove[maxDepth - curstep][j];
//            /*if (stepFind[j].x == 0 && stepFind[j].y == 0)
//            {
//                break;
//            }*/
//        }
//
//        //初始层只能走一步，那直接走就行了，不用搜索了
//        if (curstep == 0 && count == 1)
//        {
//            bestCmd = stepFind[0];
//            return 520;
//        }
//        ////我不能动了，直接判负
//        //if (count == 0)
//        //{
//        //    return -2147483646 + 1;
//        //}
//
//        //通过这个函数，完成主要变例，把大哥放到一号位，先搜大哥
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
//                //没被裁剪才去变成大哥走法
//                if (Talpha < Tbeta)
//                {
//                    //产生大哥走法
//                    pRoute->step[0] = stepFind[i];
//                    //把从step开始的num个东西整体后移一位
//                    memcpy(pRoute->step + 1, route.step, route.numMove * sizeof(struct Command));
//                    pRoute->numMove = route.numMove + 1;
//                }
//                //假如递归回来了就把它存下来
//                if (curstep == 0)
//                {
//                    bestCmd = stepFind[i];
//                }
//            }
//            if (Talpha >= Tbeta)
//            {
//                //初始化及break
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
//        //没找到能跳的情况，找move的法子
//        if (count == 0)
//        {
//            count = creatMove(board, maxDepth - curstep, curstep % 2);
//        }
//        //转换过来给存起来
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
//            //对手的棋盘操作
//            place(stepFind[i]);
//            int b = minmax(curstep + 1, MY_FLAG, Talpha, Tbeta, &route, maxDepth);
//            if (b < Tbeta)
//            {
//                Tbeta = b;
//                //没被裁剪才去变成大哥走法
//                if (Talpha < Tbeta)
//                {
//                    //产生大哥走法
//                    pRoute->step[0] = stepFind[i];
//                    //把从step开始的num个东西整体后移一位
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
    这个nowFlag是myflag或者enemyflag
    nowFlag是1或者2表示对方和我方
*/
int negaMax(int depth, int alpha, int beta, const char boardLast[BOARD_SIZE][BOARD_SIZE], struct Route* pRoute, int nowFlag)
{
    struct Route route;
    struct Command stepFind[24];
    int score;
    char boardNext[BOARD_SIZE][BOARD_SIZE];
    int count = 0;
    memcpy(boardNext, boardLast, sizeof(boardNext));

    //到了叶子节点，调用估值函数
    if (depth == 0)
    {
        //赋初值实际上是在这里
        pRoute->numMove = 0;
        return evaluateX(boardNext);
    }

    //走子
        /* BUG，跳的时候只能跳一个，没比较，且count没给赋值 */
    count = createJump(boardNext, nowFlag % 2, depth);
    //没找到能跳的情况，找move的法子
    if (count == 0)
    {
        count = creatMove(boardNext, depth, nowFlag % 2);
    }
    //转换过来给存起来
    for (int j = 0; j < 24; j++)
    {
        stepFind[j] = tempMove[depth][j];
    }

    //还能走
    if (count != 0)
    {

        //尝试去找大哥走法，主要变例优化剪枝的裁剪效率
        sortStep(stepFind, depth, count);

        //遍历所有的去找下一层
        for (int i = 0; i < count; i++)
        {
            memcpy(boardNext, boardLast, sizeof(boardNext));
            place(boardNext, stepFind[i]);
            score = -negaMax(depth - 1, -beta, -alpha, (const char(*)[BOARD_SIZE])boardNext, &route, 3 - nowFlag);

            //时间到了就返回
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
                //把从step开始的num个东西整体后移一位
                memcpy(pRoute->step + 1, route.step, route.numMove * sizeof(struct Command));
                pRoute->numMove = route.numMove + 1;
            }
        }
        return alpha;
    }

    //不能走了，被卡死了，直接返回
    else
    {
        return -214748364;
    }
}

/**
 * 轮到你落子。
 * 棋盘上0表示空白，1表示黑棋，2表示白旗
 * me表示你所代表的棋子(1或2)
 * 你需要返回一个结构体Command，其中numStep是你要移动的棋子经过的格子数（含起点、终点），
 * x、y分别是该棋子依次经过的每个格子的横、纵坐标
 * 迭代加深搜索
 */
struct Command aiTurn(const char board[BOARD_SIZE][BOARD_SIZE], int me)
{  
    initMoveTry();
    struct Route tempFirstRoute;
    //一层一层，迭代加深
    for (searchDepth = 2; (searchDepth <= DEPTH && searchDepth <= 2 * (MAX_ROUND - nowRound)); searchDepth += 2)
    {
        //给tempFirstRoute置零
        memset(&tempFirstRoute, 0, sizeof(struct Route));
        negaMax(searchDepth, -2147483647, 2147483647, board, &tempFirstRoute, MY_FLAG);
        //时间到了立即停止
        if (timeUp == 1)
        {
            break;
        }
        //把temp的给真的全局变量存进去
        memcpy(&firstRoute, &tempFirstRoute, sizeof(struct Route));     
    }
    //时间满置为假
    timeUp = 0;

    return firstRoute.step[0];
}

/**
 * 你的代码结束
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
    //先全部初始化为0
    memset(board, 0, sizeof(board));
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 8; j += 2)
        {
            //实现交错打标记的效果，上边三排交替打上叉子(2)
            board[i][j + (i + 1) % 2] = MY_FLAG;
        }
    }
    for (int i = 5; i < 8; i++)
    {
        for (int j = 0; j < 8; j += 2)
        {
            //同理下三排打上圆圈(1)
            board[i][j + (i + 1) % 2] = ENEMY_FLAG;
        }
    }
    //对自己的AI进行一些操作行为，本demo是设置了个自己的值是12
    initAI(flag);
}

void turn()
{
    //当前回合数加一
    nowRound++;
    startTime = GetTickCount();
    // AI
    //这个command里边记录了aiTurn中找到的，要走的路径
    struct Command command = aiTurn((const char(*)[BOARD_SIZE])board, myFlag);
    place(board, command);
    //如果自己是黑子，反转，然后给外边反馈
    rotateCommand(&command);
    //输出反馈结果
    printf("%d", command.numStep);
    for (int i = 0; i < command.numStep; i++)
    {
        printf(" %d,%d", command.x[i], command.y[i]);
    }
    printf("\n");
    fflush(stdout);
}
//结束函数
void end(int x)
{
    exit(0);
}

void loop()
{
    //用于接收系统给的指令，在整局游戏进行前初始化，游戏中该部分只调用一次
    char tag[10] = { 0 };
    //这个command里存对方走子的信息
    struct Command command =
    {
        .x = {0},
        .y = {0},
        .numStep = 0
    };
    int status;
    while (TRUE)
    {
        //把这个大小的地方全部初始化填为0
        memset(tag, 0, sizeof(tag));
        scanf("%s", tag);
        //收到的东西是START开始标志
        if (strcmp(tag, START) == 0)
        {
            //给myflag输入东西，表明己方的棋子颜色
            scanf("%d", &myFlag);
            //进行一些棋盘啥的初始化操作
            start(myFlag);
            //给测试系统回复一个OK表示收到
            printf("OK\n");
            //刷新标准输出
            fflush(stdout);
        }
        //收到的指令是PLACE，表示将要接收对手的行棋
        else if (strcmp(tag, PLACE) == 0)
        {
            //接收对方移动的步数
            scanf("%d", &command.numStep);
            //根据对方移动的步数进行读入操作，始终从0开始往里塞数据
            for (int i = 0; i < command.numStep; i++)
            {
                scanf("%d,%d", &command.x[i], &command.y[i]);
            }
            //如果自己是黑方，通过这一步转换为白色方的走子路径
            rotateCommand(&command);
            //操作棋盘
            place(board,command);
        }
        //收到TURN表示轮到己方走子
        else if (strcmp(tag, TURN) == 0)
        {
            initMoveTry();
            //进行己方走子行为
            turn();
        }
        //收到END指令表示结束标记
        else if (strcmp(tag, END) == 0)
        {
            //读取掉系统给的东西，结束
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
