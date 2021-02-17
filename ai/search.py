"""
COMP30024 Artificial Intelligence, Semester 1 2019
Solution to Project Part A: Searching

Authors: Adam Schlicht & Zac  Pullman
"""

import sys
import json

#Node class for tree
class Node:
    def __init__(self, state, parent, depth, dist, moveType, moveFrom, moveTo, next=None):
        self.state = state
        self.children = []
        self.next=next
        self.parent=parent
        self.depth=depth
        self.dist=dist
        self.cost=self.dist+self.depth
        self.moveType=moveType
        self.pieceMovedFrom=moveFrom
        self.pieceMovedTo=moveTo

    #add to list of children
    def add_child(self, obj):
        self.children.append(obj)

#calculate the total distance of pieces from finishing for
#each state and returns it
def calcDist(state, colour):

    dist = 0
    for i in state.keys():
        if state[i]==colour and colour == 'r':
            dist += (4-i[0])
        if state[i]==colour and colour == 'g':
            dist += (4-i[1])
        if state[i]==colour and colour == 'b':
            dist += (i[0]+i[1]+4)

    return dist

#create the root node according to the the information given
#in the json file and return it
def createInitialState(data):

    state={}
    colour = data["colour"][0]

    for i in data.get("blocks"):
        state[(i[0],i[1])]="x"

    for i in data.get("pieces"):
        state[(i[0],i[1])] =  colour

    #initiate root node with no parents
    root = Node(state, None, 0, calcDist(state, colour), "Start", None, None)

    return root

#checks if position is a valid position for a piece to
# move to off the board and hence exit
def checkIfValidOffBoard(locatation, colour):
    if (colour) == 'r':
        if locatation[0] > 3 and locatation[1] >= -3 and locatation[1] <= 3:
            return True

    if (colour) == 'g':
        if locatation[1] > 3 and locatation[0] <= 0 and locatation[0] >= -3:
            return True

    if (colour) == 'b':
        if (locatation[0] + locatation[1] < -3) and locatation[0]<=0 and locatation[0]>=-3:
            return True
    return False

#checks if location is valid position on board or valid position
#to exit to
def checkIfValidPos(locatation, colour, state):



    if locatation[0]>3 or locatation[0]<-3 or locatation[1]>3 or locatation[1]<-3 or (locatation[0]+locatation[1])<-3 or (locatation[0]+locatation[1])>3:

        if checkIfValidOffBoard(locatation, colour) and locatation not in state.keys():
                return True

        return False

    return True

#returns a list of tuples which are the possible moves and the move type
def possibleMoves(locatation, state, colour):

    possMoves = []
    for i in range(-1,2):
        for j in range(-1, 2):
            if i != j:
                stepMove=(locatation[0]+i,locatation[1]+j)
                jumpMove=(locatation[0]+2*i,locatation[1]+2*j)
                if stepMove not in state.keys() and checkIfValidPos(stepMove, colour, state):
                    possMoves.append(((locatation[0]+i,locatation[1]+j),"MOVE"))

                elif (jumpMove) not in state.keys() and checkIfValidPos(jumpMove, colour, state):
                    if checkIfValidOffBoard(jumpMove, colour)==False:
                        possMoves.append(((locatation[0]+2*i,locatation[1]+2*j),"JUMP"))

    return possMoves

#checks if a piece has completed and removes it from the dictionary
def checkIfPieceComplete(state, locatation):
    if checkIfValidOffBoard(locatation, state[locatation]):
        state.pop(locatation)

        return True

    return False

#expands node and adds children to parent node list
def expandNode(node, data):

    count=1
    for i in node.state.keys():
        if node.state[i][0]==data["colour"][0]:

            #create list of possible moves
            possMoves=possibleMoves(i, node.state, data["colour"][0])

            #create new state
            for j in possMoves:

                newState=node.state.copy()
                newState[j[0]]=newState.pop(i)

                #check if piece is completed and remove it if it is
                if checkIfPieceComplete(newState, j[0]):
                    j=(j[0],"EXIT")

                #create new node and add node to parent node
                childNode=Node(newState, node, node.depth+1, calcDist(newState, data["colour"][0]), j[1], i, j[0])
                node.add_child(childNode)



    return

#run A* search algoritm starting with the root node
def AStar(root, data):

    priorityQueue=[]
    visitedQueue = []
    priorityQueue.append((root.cost,root))
    visitedQueue.append(root)
    goalReached=0
    current=root
    expandNode(root, data)

    while(goalReached==0):

        #sort the priority queue and set the current to the first element and expand it
        priorityQueue.sort(key=lambda x: x[0])

        for i in priorityQueue:

            if i[1] not in visitedQueue:
                current = i[1]
                #print("yes")

                #add current node to visited queue
                visitedQueue.append(current)

                # find the nodes children
                expandNode(current, data)
                break



        #print(curr.dist)


        #add the children to the priority queue if they have not already been seen
        for i in current.children:
            if i not in visitedQueue:
                priorityQueue.append((i.cost,i))
               # print("added")

        #check if the goal state has been reached
        if current.dist == 0:
            goalReached = 1

    return current

#print list of moves and in correct format using the final node
def printMoves(node):

    #add nodes in path to a list
    nodeList=[]
    current=node
    while (current!=None):
        nodeList.append(current)
        current=current.parent

    #reverse the list
    nodeList.reverse()

    #print each move
    for i in nodeList:

        if i.moveType=="EXIT":
            print(i.moveType + " from {0}.".format(i.pieceMovedFrom))
        elif i.moveType == "MOVE" or i.moveType=="JUMP":
            print(i.moveType + " from {0} to {1}.".format(i.pieceMovedFrom, i.pieceMovedTo))
    return


def main():

    #with open(sys.argv[1]) as file:
    with open('test.json') as file:
        data = json.load(file)

    # TODO: Search for and output winning sequence of moves
    # ...

    #create node representing initial state
    root = createInitialState(data)

    #find the goal state
    end_node = AStar(root, data)

    #print the list of moves
    printMoves(end_node)



def print_board(board_dict, message="", debug=False, **kwargs):
    """
    Helper function to print a drawing of a hexagonal board's contents.

    Arguments:

    * `board_dict` -- dictionary with tuples for keys and anything printable
    for values. The tuple keys are interpreted as hexagonal coordinates (using
    the axial coordinate system outlined in the project specification) and the
    values are formatted as strings and placed in the drawing at the corres-
    ponding location (only the first 5 characters of each string are used, to
    keep the drawings small). Coordinates with missing values are left blank.

    Keyword arguments:

    * `message` -- an optional message to include on the first line of the
    drawing (above the board) -- default `""` (resulting in a blank message).
    * `debug` -- for a larger board drawing that includes the coordinates
    inside each hex, set this to `True` -- default `False`.
    * Or, any other keyword arguments! They will be forwarded to `print()`.
    """

    # Set up the board template:
    if not debug:
        # Use the normal board template (smaller, not showing coordinates)
        template = """# {0}
#           .-'-._.-'-._.-'-._.-'-.
#          |{16:}|{23:}|{29:}|{34:}|
#        .-'-._.-'-._.-'-._.-'-._.-'-.
#       |{10:}|{17:}|{24:}|{30:}|{35:}|
#     .-'-._.-'-._.-'-._.-'-._.-'-._.-'-.
#    |{05:}|{11:}|{18:}|{25:}|{31:}|{36:}|
#  .-'-._.-'-._.-'-._.-'-._.-'-._.-'-._.-'-.
# |{01:}|{06:}|{12:}|{19:}|{26:}|{32:}|{37:}|
# '-._.-'-._.-'-._.-'-._.-'-._.-'-._.-'-._.-'
#    |{02:}|{07:}|{13:}|{20:}|{27:}|{33:}|
#    '-._.-'-._.-'-._.-'-._.-'-._.-'-._.-'
#       |{03:}|{08:}|{14:}|{21:}|{28:}|
#       '-._.-'-._.-'-._.-'-._.-'-._.-'
#          |{04:}|{09:}|{15:}|{22:}|
#          '-._.-'-._.-'-._.-'-._.-'"""
    else:
        # Use the debug board template (larger, showing coordinates)
        template = """# {0}
#              ,-' `-._,-' `-._,-' `-._,-' `-.
#             | {16:} | {23:} | {29:} | {34:} |
#             |  0,-3 |  1,-3 |  2,-3 |  3,-3 |
#          ,-' `-._,-' `-._,-' `-._,-' `-._,-' `-.
#         | {10:} | {17:} | {24:} | {30:} | {35:} |
#         | -1,-2 |  0,-2 |  1,-2 |  2,-2 |  3,-2 |
#      ,-' `-._,-' `-._,-' `-._,-' `-._,-' `-._,-' `-.
#     | {05:} | {11:} | {18:} | {25:} | {31:} | {36:} |
#     | -2,-1 | -1,-1 |  0,-1 |  1,-1 |  2,-1 |  3,-1 |
#  ,-' `-._,-' `-._,-' `-._,-' `-._,-' `-._,-' `-._,-' `-.
# | {01:} | {06:} | {12:} | {19:} | {26:} | {32:} | {37:} |
# | -3, 0 | -2, 0 | -1, 0 |  0, 0 |  1, 0 |  2, 0 |  3, 0 |
#  `-._,-' `-._,-' `-._,-' `-._,-' `-._,-' `-._,-' `-._,-'
#     | {02:} | {07:} | {13:} | {20:} | {27:} | {33:} |
#     | -3, 1 | -2, 1 | -1, 1 |  0, 1 |  1, 1 |  2, 1 |
#      `-._,-' `-._,-' `-._,-' `-._,-' `-._,-' `-._,-'
#         | {03:} | {08:} | {14:} | {21:} | {28:} |
#         | -3, 2 | -2, 2 | -1, 2 |  0, 2 |  1, 2 | key:
#          `-._,-' `-._,-' `-._,-' `-._,-' `-._,-' ,-' `-.
#             | {04:} | {09:} | {15:} | {22:} |   | input |
#             | -3, 3 | -2, 3 | -1, 3 |  0, 3 |   |  q, r |
#              `-._,-' `-._,-' `-._,-' `-._,-'     `-._,-'"""

    # prepare the provided board contents as strings, formatted to size.
    ran = range(-3, +3 + 1)
    cells = []
    for qr in [(q, r) for q in ran for r in ran if -q - r in ran]:
        if qr in board_dict:
            cell = str(board_dict[qr]).center(5)
        else:
            cell = "     "  # 5 spaces will fill a cell
        cells.append(cell)

    # fill in the template to create the board drawing, then print!
    board = template.format(message, *cells)
    print(board, **kwargs)


# when this module is executed, run the `main` function:
if __name__ == '__main__':
    main()
