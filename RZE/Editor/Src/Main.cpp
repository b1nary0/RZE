#include <RZE.h>

class Editor : public RZE_Game
{
public:
	Editor() {}
	virtual ~Editor() {}

	virtual void Start()
	{
		RZE_Game::Start();
	}
	virtual void Update()
	{

	}
};

int main(void)
{
	Functor<RZE_Game* const> gameCreateFunc([]()
	{
		return new Editor();
	});

	RZE_Engine::Get()->Run(gameCreateFunc);

	return 0;
}