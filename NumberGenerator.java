public class NumberGenerator{
	private int currentNumber = 1;
	public int getNext(){
		int result = currentNumber;
		currentNumber++;
		return result;
	}
}