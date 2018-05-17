#include <iostream>
#include <random>
#include <vector>
#include <thread>
#include <random>
#include <condition_variable>
#include <queue>
#include <algorithm>

class Student{


public:
    explicit Student(std::string basic_string) :name(basic_string){

    }

    std::string name;
    std::vector<int> marks;
    int marks_sum = 0;

    void add_mark(int mark){
        marks.push_back(mark);
        marks_sum += mark;
    }

    int get_score(){
        if(marks.empty()) return  0;
        return static_cast<int>(marks_sum / marks.size());
    }
};

struct MarkData{
    MarkData(int si, int sm) :
    student_index(si), student_mark(sm){
    }

    int student_index;
    int student_mark;
};

class Subject{

    std::string name;
    int exams_num;

    const int students_num;
    std::condition_variable &cv;
    std::queue<MarkData> &marks_buffer;
    std::mutex &mx;
    int &total_running;

public:

    Subject(std::string name, const int students_num, std::queue<MarkData> &marks_buffer, int &total_running,
            std::condition_variable &cv, std::mutex &mx, int exams_num=INT32_MAX):
    name(std::move(name)), exams_num(exams_num),
    students_num(students_num), marks_buffer(marks_buffer),
    cv(cv), mx(mx), total_running(total_running){}

    void start_subject(){
        for (int i = 0; i < exams_num; i++) {
            std::vector<int> indices(students_num);
            for(int dc = 0; dc < indices.size(); dc++) indices[dc] = dc;
            std::shuffle(indices.begin(), indices.end(), std::mt19937(std::random_device()()));
            for (int si = 0; si< indices.size(); si++) {
                {
                    std::lock_guard<std::mutex> lk(mx);
                    marks_buffer.push(MarkData(indices[si], (int)random()%100));
                }
                cv.notify_all();

                // rand sleep between students evaluation
                auto sleep_time = std::chrono::milliseconds(random() % 20);
                std::this_thread::sleep_for(sleep_time);
            }
        }
        {
            std::lock_guard<std::mutex> lk(mx);
            total_running--;
        }
        cv.notify_all();
    }

};

class StudyProcess {

    std::vector<Student> students;
    std::vector<Subject> subjects;
    std::vector<std::thread> subjects_threads;
    const int subjects_num;

    std::queue<MarkData> marks_buffer;
    std::condition_variable cv;
    std::mutex mx;
    int total_running;

    const int students_num = 20;

    void print_students_data(){
        for(auto st : students){
            std::cout<<st.name<<" "<<st.marks_sum<<" "<<st.get_score()<<'\n';
        }
    }

public:

    explicit StudyProcess(int subjects_num):subjects_num(subjects_num), total_running(0){
        for(int i = 0; i < students_num; i++) {
            students.emplace_back("Name_" + std::to_string(i));
        }
    }

    void start_study(){
        for(int i = 0; i < subjects_num; i++){
            {
                std::lock_guard<std::mutex> lk(mx);
                total_running++;
            }
            Subject sbj("Subject_"+ std::to_string(i), students_num, marks_buffer, total_running, cv, mx);
            subjects.emplace_back(sbj);
            subjects_threads.push_back(std::thread(&Subject::start_subject, subjects.back()));
        }
        while (true) {
            {
                std::unique_lock<std::mutex> lk(mx);
                cv.wait(lk);
                if (!marks_buffer.empty()) {
                    const auto mark_data = marks_buffer.front();
                    marks_buffer.pop();
                    students[mark_data.student_index].add_mark(mark_data.student_mark);
                }
                if(total_running==0){
                    print_students_data();
                    break;
                }
            }
            print_students_data();
        }
        for(int i = 0; i<subjects.size(); i++){
            subjects_threads[i].join();
        }
    }



};



int main() {
    StudyProcess sp(8);
    sp.start_study();
}