package com.ssafy.view;
import java.awt.BorderLayout;
import java.awt.GridLayout;
import java.awt.Image;
import java.awt.TextField;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.File;
import java.io.IOException;
import java.util.List;

import javax.imageio.ImageIO;
import javax.swing.ImageIcon;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JFrame;
import javax.swing.JLabel;

import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.table.DefaultTableModel;

import com.ssafy.service.FoodServiceImpl;
import com.ssafy.service.FoodService;
import com.ssafy.vo.Food;
import com.ssafy.vo.FoodPageBean;
import com.ssafy.vo.SafeFoodException;


public class FoodInfoView{
	
	/**model들 */
	private FoodService 		foodService;
	
	/** main 화면 */
	private JFrame frame;
	
	/** 섭취 화면*/
	JFrame owner;
	
	/**섭취 내역 추가 버튼*/
	private JButton  addBt;
	/**창 닫기 버튼*/
	private JButton  closeBt;
	/**상품 이미지 표시 Panel*/
	private JLabel	 imgL;
	private JLabel[] foodInfoL;
	private JLabel[] nutritionL;
	private JTextField numberTf;
	/**조회 조건*/
	private JComboBox<String> findC; 
	private TextField		  wordTf;
	private JButton			  searchBt;
	
	/**조회 내용 표시할 table*/
	private DefaultTableModel foodModel;
	private JTable			  foodTable;
	private JScrollPane		  foodPan;
	private String[]		  title={"번호","상품명","제조사"};
	/**검색  조건*/
	private String	key;
	/**검색할 단어*/
	private String  word;
	private String[] choice= {"all","name","maker","material"};
	/**화면에 표시하고 있는 상품*/
	private Food curfood;
	

	

	private ActionListener buttonHandler = new ActionListener() {
		@Override
		public void actionPerformed(ActionEvent e) {
			Object source = e.getSource();
			try {
				if(source == closeBt) { 
					frame.setVisible(false);
				}else if(source == searchBt) { 
					searchFoods();
				}
			} catch (SafeFoodException ue) {
			     ue.printStackTrace();	
			}
		}
	};
	MouseAdapter handler = new MouseAdapter() {
		@Override
		public void mouseClicked(MouseEvent e) {
			int row =  foodTable.getSelectedRow();
			System.out.println("선택된 row :"+row);
			System.out.println("선택된 row의 column 값 :"+foodModel.getValueAt(row, 0));
			try{
				int code = Integer.parseInt(((String)foodModel.getValueAt(row, 0)).trim());
				showFoodInfo(code);
				
			}catch(SafeFoodException ue){
			}
			
		}

		
	};

	private void showFoodInfo(int code) {
		curfood = foodService.search(code);
		nutritionL[0].setText(curfood.getSupportpereat()+"g");
		nutritionL[1].setText(curfood.getCalory()+"Kcal");
		nutritionL[2].setText(curfood.getCarbo()+"g");
		nutritionL[3].setText(curfood.getProtein()+"");
		nutritionL[4].setText(curfood.getFat()+"g");
		nutritionL[5].setText(curfood.getSugar()+"");
		nutritionL[6].setText(curfood.getNatrium()+"");
		nutritionL[7].setText(curfood.getChole()+"");
		nutritionL[8].setText(curfood.getFattyacid()+"");
		nutritionL[9].setText(curfood.getTransfat()+"");
		
		//foodInfoL[0].setText(""+curfood.getCode());
		foodInfoL[0].setText(curfood.getName());
		foodInfoL[1].setText(curfood.getMaker());
		foodInfoL[2].setText(curfood.getMaterial());
		foodInfoL[3].setText(curfood.getAllergy());
		
		Image img = null;
		try {                
			img = ImageIO.read(new File(curfood.getImg()));
         } catch (IOException ex) {
        	 ex.printStackTrace();
         }
		img = img.getScaledInstance(200, 200, Image.SCALE_SMOOTH);
		imgL.setIcon(new ImageIcon(img));
	}
	public FoodInfoView(){
		/*Service들 생성 */
		foodService = new FoodServiceImpl();
		/*메인 화면 설정 */
		frame = new JFrame("Safe Food -- 상품 정보");
		frame.addWindowListener(new WindowAdapter() {
			public void windowClosing(WindowEvent e){
				frame.dispose();
			}
		});
		setMain();
		frame.setSize(1000, 400);
		frame.setResizable(false);
		frame.setVisible(true);
		showFoodInfo(1);
		showFoods();
	}

	/**메인 화면인 상품 목록을 위한 화면 셋팅하는 메서드  */
	public void setMain(){
		/*왼쪽 화면을 위한 설정 */
		JPanel left = new JPanel(new BorderLayout());
		JPanel leftCenter = new JPanel(new GridLayout(1, 2));
		JPanel leftR = new JPanel(new GridLayout(10, 2));
		String[] nut= {"총 용량","칼로리","탄수화물","단백질","지방","당류","나트륨","콜레스테롤","포화 지방산","트렌스지방"};
		int size = nut.length;
		JLabel nutLabel[] = new JLabel[size];
		nutritionL = new JLabel[size];
		for (int i = 0; i <size; i++) {
			nutLabel[i] = new JLabel(nut[i]);
			nutritionL[i]=new JLabel("");
			leftR.add(nutLabel[i]);
			leftR.add(nutritionL[i]);
		}
		imgL = new JLabel();
		leftCenter.add(imgL);
		leftCenter.add(leftR);
		
		JPanel leftBottom = new JPanel(new GridLayout(5, 2));
		String[] foods= {"이름","제조사","원재료","알레르기"};
		int foodSize = foods.length;
		JLabel[] foodLabel = new JLabel[foodSize];
		foodInfoL = new JLabel[foodSize];
		for (int i = 0; i <foodSize; i++) {
			foodLabel[i] = new JLabel(foods[i]);
			foodInfoL[i] = new JLabel("");
			leftBottom.add(foodLabel[i]);
			leftBottom.add(foodInfoL[i]);
		}
		numberTf = new JTextField();
		addBt = new JButton("섭취 추가");
		JPanel temp = new JPanel(new GridLayout(1,2));
		temp.add(new JLabel("섭취 수량"));
		temp.add(numberTf);
		//leftBottom.add(temp);
		//leftBottom.add(addBt);
		
		left.add(new JLabel("상품 정보", JLabel.CENTER),"North");
		left.add(leftCenter,"Center");
		left.add(leftBottom,"South");
		
		
		/*오른쪽 화면을 위한 설정 */
		JPanel right = new JPanel(new BorderLayout());
		
		JPanel rightTop = new JPanel(new GridLayout(1, 3));
		String[] item ={"---all---","이름","제조사","재료"}; 
		findC = new JComboBox<String>(item);
		wordTf= new TextField();
		searchBt= new JButton("검색");
		rightTop.add(findC);
		rightTop.add(wordTf);
		rightTop.add(searchBt);
		
		JPanel rightCenter = new JPanel(new BorderLayout());
		foodModel = new DefaultTableModel(title,20);
		foodTable = new JTable(foodModel);
		foodPan = new JScrollPane(foodTable);
		foodTable.setColumnSelectionAllowed(true);
		rightCenter.add(new JLabel("상품 목록", JLabel.CENTER),"North");
		rightCenter.add(foodPan,"Center");
		
		right.add(rightTop,"North");
		right.add(rightCenter,"Center");
		
		JPanel mainP = new JPanel(new GridLayout(1, 2));
		mainP.add(left);
		mainP.add(right);
		frame.add(mainP,"Center");
		
		/*이벤트 연결*/
		foodTable.addMouseListener(handler);
		addBt.addActionListener(buttonHandler);
		searchBt.addActionListener(buttonHandler);
//		closeBt.addActionListener(buttonHandler);
		showFoods();
	}
	
	
	/**검색 조건에 맞는 상품 정보 검색 */
	private void searchFoods() {
		word = wordTf.getText().trim();
		key = choice[findC.getSelectedIndex()];
		System.out.println("word:"+word+" key:"+key);
		showFoods();		
	}
	/**
	 * 섭취 목록을  갱신하기 위한 메서드 
	 * @param list
	 */
	public void showFoods(){
		List<Food> foods = foodService.searchAll(new FoodPageBean(key, word, null, 1));
		if(foods!=null){
			int i=0;
			String[][]data = new String[foods.size()][3];
			for (Food food : foods) {
				data[i][0]= ""+food.getCode();
				data[i][1]= food.getName();
				data[i++][2]= food.getMaker();
			}
			foodModel.setDataVector(data, title);
		}
	}
	public static void main(String[] args) {
		new FoodInfoView();
	}
	
}

