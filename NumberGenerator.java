public class NumberGenerator{
	private int number = 1;
	public NumberGenerator(){
		System.out.println("Creating NmbGen ");
	}
	public int getNext(){
		int result = number;
		this.number++;
		System.out.println("From NmbGen " + result);
		return result;
	}
}//class