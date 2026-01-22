#include "RefactorTool.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/Tooling.h"
#include <gtest/gtest.h>
#include <string>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

std::string applyRefactorings(const std::string &Code) {
    std::string ResultCode = Code;

    class TestAction : public clang::ASTFrontendAction {
    public:
        TestAction(std::string &OutResult) : OutResult(OutResult) {}

        std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef) override {
            TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
            return std::make_unique<ComplexConsumer>(TheRewriter);
        }

        void EndSourceFileAction() override {
            const clang::SourceManager &SM = TheRewriter.getSourceMgr();
            clang::FileID ID = SM.getMainFileID();
            if (const llvm::RewriteBuffer *Buf = TheRewriter.getRewriteBufferFor(ID)) {
                OutResult = std::string(Buf->begin(), Buf->end());
            }
        }

    private:
        clang::Rewriter TheRewriter;
        std::string &OutResult;
    };

    std::vector<std::string> Args = {"-std=c++17", "-fno-delayed-template-parsing"};
    bool Success =
        clang::tooling::runToolOnCodeWithArgs(std::make_unique<TestAction>(ResultCode), Code, Args, "input.cc");

    return Success ? ResultCode : Code;
}

TEST(VirtualDtorTest, AddsVirtualWhenDerivedExists) {
    constexpr const char *Input = R"cpp(
class Base { ~Base() {} };
class Derived : public Base {};
)cpp";

    constexpr const char *Expected = R"cpp(
class Base { virtual ~Base() {} };
class Derived : public Base {};
)cpp";

    EXPECT_EQ(applyRefactorings(Input), Expected);
}

TEST(VirtualDtorTest, DoesNotAddVirtualToStandaloneClass) {
    constexpr const char *Input = R"cpp(
class A { ~A() {} };
)cpp";

    EXPECT_EQ(applyRefactorings(Input), Input);
}

TEST(OverrideTest, AddsOverrideToOverridingMethod) {
    constexpr const char *Input = R"cpp(
class B { virtual void f(); };
class D : public B { void f() {} };
)cpp";

    constexpr const char *Expected = R"cpp(
class B { virtual void f(); };
class D : public B { void f() override {} };
)cpp";

    EXPECT_EQ(applyRefactorings(Input), Expected);
}

TEST(OverrideTest, DoesNotAddOverrideToNonVirtualMethod) {
    constexpr const char *Input = R"cpp(
class X { void g() {} };
)cpp";

    EXPECT_EQ(applyRefactorings(Input), Input);
}

TEST(RangeForTest, AddsAmpersandToConstStdString) {
    constexpr const char *Input = R"cpp(
#include <string>
#include <vector>
void foo() {
    std::vector<std::string> v;
    for (const std::string s : v) {}
}
)cpp";

    constexpr const char *Expected = R"cpp(
#include <string>
#include <vector>
void foo() {
    std::vector<std::string> v;
    for (const std::string& s : v) {}
}
)cpp";

    EXPECT_EQ(applyRefactorings(Input), Expected);
}

TEST(RangeForTest, DoesNotAddAmpersandToInt) {
    constexpr const char *Input = R"cpp(
#include <vector>
void foo() {
    std::vector<int> v;
    for (const int x : v) {}
}
)cpp";

    EXPECT_EQ(applyRefactorings(Input), Input);
}

TEST(RefactoringTest, MultipleChangesInOneFile) {
    constexpr const char *Input = R"cpp(
class Base {
public:
    ~Base() {}
    virtual void foo() {}
};

class Derived : public Base {
public:
    void foo() {}
};

#include <vector>
#include <string>
void test() {
    std::vector<std::string> vec;
    for (const std::string item : vec) {}
}
)cpp";

    constexpr const char *Expected = R"cpp(
class Base {
public:
    virtual ~Base() {}
    virtual void foo() {}
};

class Derived : public Base {
public:
    void foo() override {}
};

#include <vector>
#include <string>
void test() {
    std::vector<std::string> vec;
    for (const std::string& item : vec) {}
}
)cpp";

    EXPECT_EQ(applyRefactorings(Input), Expected);
}

TEST(RefactoringTest, NoChangesForCorrectCode) {
    constexpr const char *Input = R"cpp(
class GoodBase {
public:
    virtual ~GoodBase() {}
    virtual void method() {}
};

class GoodDerived : public GoodBase {
public:
    void method() override {}
};

#include <vector>
#include <string>
void test() {
    std::vector<std::string> vec;
    for (const std::string& item : vec) {}
}
)cpp";

    // Код уже правильный - не должно быть изменений
    EXPECT_EQ(applyRefactorings(Input), Input);
}

// Проверяем, что шаблонные базовые классы с наследниками тоже получают virtual деструктор
TEST(RefactoringTest, TemplateClassWithDestructor) {
    constexpr const char *Input = R"cpp(
template<typename T>
class Base {
public:
    ~Base() {}
};

class Derived : public Base<int> {};
)cpp";

    constexpr const char *Expected = R"cpp(
template<typename T>
class Base {
public:
    virtual ~Base() {}
};

class Derived : public Base<int> {};
)cpp";

    EXPECT_EQ(applyRefactorings(Input), Expected);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

    // Используем _exit вместо exit/return, чтобы избежать ошибок при финализации
    // глобальных объектов LLVM/ClVM (например, "pure virtual method called",
    // "free(): invalid pointer"), которые возникают из-за неправильного порядка
    // уничтожения статических объектов в shared-библиотеках Clang/LLVM.
    // Это безопасно в юнит-тестах, так как все ресурсы уже обработаны,
    // а graceful shutdown не требуется.
    _exit(result);
}