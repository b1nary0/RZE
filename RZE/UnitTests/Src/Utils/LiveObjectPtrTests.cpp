#include <doctest/doctest.h>

#include <Utils/LiveIDAllocator.h>
#include <Utils/LiveObjectPtr.h>

#include <type_traits>
#include <utility>

namespace
{
	LiveIDAllocator& TestAllocator()
	{
		static LiveIDAllocator s_allocator;
		return s_allocator;
	}

	// Alive from construction to destruction, like GameObject
	struct TestObject
	{
		TestObject() : ID(TestAllocator().Acquire()) {}
		~TestObject() { TestAllocator().Release(ID); }

		U64 ID;
		int Value = 0;
	};

	struct TestLifetime
	{
		static bool IsAlive(U64 id) { return TestAllocator().Contains(id); }
		static U64 GetID(const TestObject* object) { return object->ID; }
	};

	using TestPtr = LiveObjectPtr<TestObject, TestLifetime>;
	using UniqueTestPtr = UniqueLiveObjectPtr<TestObject, TestLifetime>;
}

static_assert(std::is_copy_constructible_v<TestPtr> && std::is_copy_assignable_v<TestPtr>, "Shared handles copy");
static_assert(!std::is_copy_constructible_v<UniqueTestPtr> && !std::is_copy_assignable_v<UniqueTestPtr>, "One holder per object");
static_assert(std::is_nothrow_move_constructible_v<UniqueTestPtr> && std::is_nothrow_move_assignable_v<UniqueTestPtr>, "Handles can be handed over");
static_assert(std::is_same_v<decltype(std::declval<const UniqueTestPtr&>().operator->()), const TestObject*>, "A const holder only reads");
static_assert(!std::is_convertible_v<TestObject*, TestPtr>, "Handles are made explicitly");

TEST_CASE("Default and null handles are invalid and equal each other")
{
	const TestPtr defaulted;
	const TestPtr null = nullptr;
	const TestPtr fromNull(static_cast<TestObject*>(nullptr));
	CHECK_FALSE(defaulted.IsValid());
	CHECK(defaulted == nullptr);
	CHECK(defaulted == null);
	CHECK(null == fromNull);
	CHECK(fromNull.Get() == nullptr);
}

TEST_CASE("A handle is valid until its object is destroyed, then equals nullptr")
{
	TestPtr handle;
	{
		TestObject object;
		handle = TestPtr(&object);
		CHECK(handle.IsValid());
		CHECK(handle != nullptr);
		CHECK(handle.Get() == &object);
		handle->Value = 3;
		CHECK(object.Value == 3);
	}
	CHECK_FALSE(handle.IsValid());
	CHECK(handle == nullptr);
}

TEST_CASE("Handles are equal when they refer to the same object, and a new object at a reused address is another one")
{
	TestObject* first = new TestObject();
	const TestPtr firstHandle(first);
	CHECK(TestPtr(first) == firstHandle);

	delete first;
	// Likely the same address; either way, a new object is a different object
	TestObject* second = new TestObject();
	const TestPtr secondHandle(second);
	CHECK(secondHandle != firstHandle);
	CHECK(firstHandle == nullptr);
	CHECK(secondHandle != nullptr);
	delete second;
}

TEST_CASE("A handle equals a raw pointer to its object while the object is alive, from either side")
{
	TestObject object;
	TestObject other;
	const TestPtr handle(&object);
	const TestObject* const constObject = &object;
	CHECK(handle == &object);
	CHECK(&object == handle);
	CHECK(handle == constObject);
	CHECK(handle != &other);
	CHECK(&other != handle);

	const TestObject* const null = nullptr;
	CHECK(handle != null);
	CHECK(TestPtr() == null);
	CHECK(null == TestPtr());
}

TEST_CASE("A dead handle only equals a null pointer, even when a new object has its address")
{
	TestObject* first = new TestObject();
	const TestPtr handle(first);
	TestObject* const firstAddress = first;
	delete first;

	CHECK(handle != firstAddress);
	TestObject* second = new TestObject();
	if (second == firstAddress)
	{
		CHECK(handle != second);
	}
	const TestObject* const null = nullptr;
	CHECK(handle == null);
	delete second;
}

TEST_CASE("A unique handle compares with raw pointers like a shared one")
{
	TestObject object;
	const UniqueTestPtr holder(&object);
	CHECK(holder == &object);
	CHECK(&object == holder);
	CHECK(UniqueTestPtr() != &object);
}

TEST_CASE("Copying a handle keeps both valid")
{
	TestObject object;
	const TestPtr original(&object);
	const TestPtr copy = original;
	CHECK(original.IsValid());
	CHECK(copy.IsValid());
	CHECK(copy == original);
}

TEST_CASE("Moving a handle, by construction or assignment, leaves the source null")
{
	TestObject object;
	TestPtr source(&object);
	const TestPtr constructed = std::move(source);
	CHECK(constructed.IsValid());
	CHECK(source == nullptr);
	CHECK(source == TestPtr());

	TestPtr again(&object);
	TestPtr assigned;
	assigned = std::move(again);
	CHECK(assigned.IsValid());
	CHECK(again == TestPtr());
}

TEST_CASE("A unique handle hands its object over when moved")
{
	TestObject object;
	UniqueTestPtr holder(&object);
	CHECK(holder.IsValid());

	UniqueTestPtr next = std::move(holder);
	CHECK(next.IsValid());
	CHECK(next.Get() == &object);
	CHECK(holder == nullptr);
	CHECK(holder == UniqueTestPtr());

	UniqueTestPtr last;
	last = std::move(next);
	CHECK(last->Value == 0);
	CHECK(next == nullptr);
}
