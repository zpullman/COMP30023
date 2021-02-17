with open("C:/Users/61422/Documents/COMP30023/project_2/proj-2_common_passwords.txt") as f:
    lines = f.readlines()

fw = open("6letter_dict.txt", "w")
for line in lines:
    for word in line.split():
        if (len(word) == 6):
            fw.write(word +"\n")
