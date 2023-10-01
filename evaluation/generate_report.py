# generate_report.py
import sys
import os
import cv2

def diffImage(filename, refPath):
    img1 = cv2.imread(filename)
    img2 = cv2.imread(refPath + filename)
    diff = cv2.absdiff(img1, img2)
    cv2.imwrite("diff_" + filename, diff)

def printImages(file, filename, refPath):
    if os.path.isfile(filename):
        file.write("\\begin{figure}[h!]\n\\centering\n\\includegraphics[width=0.3\\textwidth]{" + filename + "}\n\\includegraphics[width=0.3\\textwidth]{" + refPath + filename + "}\n\\includegraphics[width=0.3\\textwidth]{diff_" + filename + "}\n\\end{figure}\n")
    else:
        file.write("Error: Some of the necessary files do not exist.")
        file.write("\\begin{figure}[h!]\n\\centering\n\\includegraphics[width=0.3\\textwidth]{owl.png}\n\\includegraphics[width=0.3\\textwidth]{" + refPath + filename + "}\n\\includegraphics[width=0.3\\textwidth]{diff_" + filename + "}\n\\end{figure}\n")

if __name__ == "__main__":
    tasks_dict = {
        "submission1": 1,
        "submission2": 2,
        "submission3": 3,
        "submission4": 4,
        "submission5": 5,
        "submission6": 6
    }
    tasks_string_dict = {
        "submission1": "task1",
        "submission2": "task2",
        "submission3": "task3",
        "submission4": "task4",
        "submission5": "task5",
        "submission6": "task6"
    }
    task_names = ["Task 1", "Task 2", "Task 3", "Task 4", "Task 5", "Task 6"]

    task_string = tasks_string_dict[sys.argv[1]]
    task = tasks_dict[sys.argv[1]]
    task_idx = task - 1

    vkExists = os.path.exists("../_project/GCGProject_VK")
    glExists = os.path.exists("../_project/GCGProject_GL")
    print("Using Vulkan: " + str(vkExists))
    print("Using OpenGL: " + str(glExists))

    refPath = ""
    if glExists:
        refPath = "GCG_GL/"
    if vkExists:
        refPath = "GCG_VK/"

    cameraPoses = ["front"]
    if task > 1:
        cameraPoses = ["front", "front_right", "right", "front_left", "left", "front_up", "up", "front_down", "down", "right_up", "right_down", "left_up", "left_down", "back"]

    with open("report.tex","w") as file:
        file.write("\\documentclass{article}\n")
        file.write("\\usepackage{graphicx}\n")
        file.write("\\usepackage{subcaption}\n")
        file.write("\\usepackage[a4paper, margin=1in]{geometry}")
        file.write("\\title{" + task_names[task_idx] + " Report}\n")
        file.write("\\begin{document}\n")
        file.write("\\maketitle")

        file.write("\\section{Results}\n")
        file.write("Side-by-side comparisons: left = your solution, middle = reference image, right = absolute difference.")

        if not vkExists and not glExists:
            file.write("We could not decide whether your taking the OpenGL or Vulkan Route. Please stick to the original folder names.\n")
    
        file.write("\\subsection{Standard View}\n")
        for cameraPos in cameraPoses:
            filename = "standard_" + cameraPos + ".png"
            diffImage(filename, refPath)
            printImages(file, filename, refPath)
        file.write("\\newpage")

        if task >= 3:
            file.write("\\subsection{Backface Culling View}\n")
            for cameraPos in cameraPoses:
                filename = "culling_" + cameraPos + ".png"
                diffImage(filename, refPath)
                printImages(file, filename, refPath)
            file.write("\\newpage")
        
        if task == 3 or task == 4:
            file.write("\\subsection{Wireframe View}\n")
            for cameraPos in cameraPoses:
                filename = "wireframe_" + cameraPos + ".png"
                diffImage(filename, refPath)
                printImages(file, filename, refPath)
            file.write("\\newpage")

            file.write("\\subsection{Wireframe and Backframe Culling View}\n")
            for cameraPos in cameraPoses:
                filename = "culling_wireframe_" + cameraPos + ".png"
                diffImage(filename, refPath)
                printImages(file, filename, refPath)
            file.write("\\newpage")

        if task == 5:
            file.write("\\subsection{Normals View}\n")
            for cameraPos in cameraPoses:
                filename = "normals_" + cameraPos + ".png"
                diffImage(filename, refPath)
                printImages(file, filename, refPath)
            file.write("\\newpage")

            file.write("\\subsection{Normals Backface Culling View}\n")
            for cameraPos in cameraPoses:
                filename = "culling_normals_" + cameraPos + ".png"
                diffImage(filename, refPath)
                printImages(file, filename, refPath)
            file.write("\\newpage")

        if task == 6:
            file.write("\\subsection{Texcoords View}\n")
            for cameraPos in cameraPoses:
                filename = "texcoords_" + cameraPos + ".png"
                diffImage(filename, refPath)
                printImages(file, filename, refPath)
            file.write("\\newpage")

            file.write("\\subsection{Texcoords Backface Culling View}\n")
            for cameraPos in cameraPoses:
                filename = "culling_texcoords_" + cameraPos + ".png"
                diffImage(filename, refPath)
                printImages(file, filename, refPath)
            file.write("\\newpage")

        file.write("\\newpage")
        file.write("\\end{document}\n")
