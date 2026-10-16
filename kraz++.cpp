#include<unordered_map>
#include<filesystem>
#include<algorithm>
#include<stdexcept>
#include<iostream>
#include<fstream>
#include<cstdint>
#include<cstdlib>
#include<climits>
#include<iomanip>
#include<sstream>
#include<cstring>
#include<cfloat>
#include<string>
#include<cctype>
#include<vector>
#include<cmath>
#include<stack>
#include<set>
using namespace std;
using namespace std::filesystem;
class CodeBase{
	private:
		int error_line_num;
		string code;
		void tokenizer(){
			token.clear();
			token.push_back({});
			int line_num=1;
			error_line_num=1;
			string temp="";
			for(int i=0;i<(int)code.size();i++){
				switch(code[i]){
					case '\n':
						line_num++;
						error_line_num++;
						if(temp.empty()){
							token.push_back({});
							break;
						}
						token.back().push_back(temp);
						token.push_back({});
						temp="";
						break;
					case ' ':
					case '\t':
					case ',':
						if(temp.empty())break;
						token.back().push_back(temp);
						temp="";
						break;
					case '`':{
						if(!temp.empty())token.back().push_back(temp);
						temp='`';
						bool find_one=0;
						for(int j=i+1;j<(int)code.size();j++){
							temp+=code[j];
							if(code[j]=='`'){
								find_one=1;
								i=j;
								break;
							}else if(code[j]=='\n')throw invalid_argument("'`'未匹配");
						}
						if(!find_one)throw invalid_argument("'`'未匹配");
						break;
					}
					case '#':
						if(temp.size())token.back().push_back(temp);
						temp="";
						while(i<(int)code.size()&&code[i]!='\n')i++;
						i--;
						break;
					case '\0':
					case '\r':
						break;
					default:
						temp+=code[i];
				}
			}
			if(temp.size())token.back().push_back(temp);
		}
		vector<int> match_pair_tos(){
			vector<string>keywords={"for","if","while","func","elif","else"};
			auto can_have=[keywords](const string&str){
				for(auto i:keywords)if(i==str)return 1;
				return 0;
			};
			vector<int> pair_to(token.size(),-1);
			for(int i=0;i<(int)token.size();i++){
				error_line_num=i+1;
				if(token[i].size()>=1&&token[i][0].size()>=1&&token[i][0][0]==':'){
					if(token[i].size()!=1)throw invalid_argument("':'行不能有其它内容");
					if(token[i][0].size()!=1)throw invalid_argument("':'行不能有其它内容");
					if(i==0)throw invalid_argument("':'不匹配");
					int count=1;
					for(int j=i-1;j>=0;j--){
						if(token[j].empty())continue;
						if(token[j].back()==":"){
							if(token[j].size()==1)count++;
							else if(can_have(token[j][0]))count--;
							if(count==0){
								pair_to[i]=j;
								pair_to[j]=i;
								break;
							}
						}
					}
					if(pair_to[i]==-1)throw invalid_argument("':'不匹配");
				}
			}
			for(int i=0;i<(int)token.size();i++){
				error_line_num=i+1;
				if(token[i].empty())continue;
				if(((token[i].size()==1&&token[i][0]==":")||can_have(token[i][0]))&&pair_to[i]==-1)throw invalid_argument("':'不匹配");
			}
			return pair_to;
		}
		unordered_map<string,int> find_funcs(){
			unordered_map<string,int> funcs;
			for(int i=0;i<(int)token.size();i++){
				error_line_num=i+1;
				if(token[i].size()){
					if(token[i][0]=="func"){
						if(token[i].size()<2)throw invalid_argument("func语法不正确");
						if(!funcs.count(token[i][1]))funcs[token[i][1]]=i;
						else throw invalid_argument("函数"+token[i][1]+"已定义");
					}
				}
			}
			return funcs;
		}
		pair<vector<pair<string,int>>,int> link_head(){
			int offset=0;
			vector<pair<string,int>>line_nums;
			unordered_map<string,int> head;
			for(int i=(int)token.size()-1;i>=0;i--){
				error_line_num=i+1;
				if(token[i].size()){
					if(token[i][0]=="include"){
						if(token[i].size()==2){
							path target=path("./head")/(token[i][1]+".hraz");
							error_code _ec;
							if(is_regular_file(target,_ec)&&!_ec&&exists(target,_ec)&&!_ec){
								ifstream in(target);
								string s{istreambuf_iterator<char>(in),{}};
								if(s.size()>=3&&(unsigned char)s[0]==0xEF&&(unsigned char)s[1]==0xBB&&(unsigned char)s[2]==0xBF)s.erase(0,3);
								int n=count(s.begin(),s.end(),'\n')+1;
								offset+=n;
								line_nums.push_back({token[i][1]+".hraz",n});
								string key=target.lexically_normal().string();
								if(head.count(key)){
									error_line_num=head[key]+1;
									throw invalid_argument("头文件:"+token[i][1]+".hraz重复包括");
								}
								head[key]=i;
								code=s+"\n"+code;
							}else throw invalid_argument("未找到文件:"+token[i][1]+".hraz");
						}else throw invalid_argument("include语法不正确");
					}
				}
			}
			reverse(line_nums.begin(),line_nums.end());
			return {line_nums,offset};
		}
	public:
		vector<vector<string>> token;
		vector<int> pair_to;
		unordered_map<string,int> funcs;
		vector<pair<string,int>> line_nums;
		int offset=0;
		void init(const string&input_code){
			try{
				code=input_code;
				tokenizer();
				auto[li,of]=link_head();
				line_nums=li;
				offset=of;
				tokenizer();
				pair_to=match_pair_tos();
				funcs=find_funcs();
			}catch(invalid_argument&e){
				if(error_line_num-offset<=0){
					int sum=0;
					for(auto i:line_nums){
						sum+=i.second;
						if(sum>=error_line_num){
							cerr<<"invalid argument: 头文件:"<<i.first<<"第"<<to_string(error_line_num-sum+i.second)<<"行,"<<e.what()<<'\n';
							break;
						}
					}
				}else cerr<<"invalid argument: 第"+to_string(error_line_num-offset)+"行,"<<e.what()<<'\n';
				exit(1);
			}
		}
		CodeBase(const string&input_code){
			init(input_code);
		}
		CodeBase(){}
};
class Value{
	private:
		long double data=0;
		signed char bits=32;
		bool sgn=true;
		pair<signed char,bool> type_info(const string& type){
			const pair<string,pair<signed char,bool>> type_len[]={
				{"char",{8,1}},
				{"int8",{8,1}},{"uint8",{8,0}},
				{"int16",{16,1}},{"uint16",{16,0}},
				{"int32",{32,1}},{"uint32",{32,0}},
				{"int64",{64,1}},{"uint64",{64,0}},
				{"float",{-32,1}},{"double",{-64,1}},
			};
			for(auto&t:type_len)if(t.first==type)return t.second;
			throw invalid_argument("未知类型\""+type+"\"");
		}
		long double narrow(long double v,signed char bits,bool sgn){
			if(bits<0){
				if(!isfinite(v))return v;
				long double ma;
				if(bits==-32)ma=FLT_MAX;
				else ma=DBL_MAX;
				if(fabs(v)>ma){
					if(v<0)return -ma;
					else return ma;
				}else{
					if(bits==-32)return(long double)(float)v;
					else return(long double)(double)v;
				}
			}
			long double p=ldexp(1.0L,bits);
			long double m=fmod(trunc(v),p);
			if(m<0)m+=p;
			if(sgn&&m>=p/2)return m-p;
			else return m;
		}
	public:
		Value()=default;
		Value(long double v,const string& t){
			auto[b,s]=type_info(t);
			bits=b;
			sgn=s;
			data=narrow(v,bits,sgn);
		}
		void turn(long double v){
			data=narrow(v,bits,sgn);
		}
		long double as_num(){
			return data;
		}
		signed char get_bits(){
			return bits;
		}
		long double narrow_like(long double v){
			return narrow(v,bits,sgn);
		}
		int64_t as_int(){
			if(!isfinite(data))throw domain_error("无法转为整数");
			long double t=trunc(data);
			if(t<-9223372036854775808.0L||t>9223372036854775807.0L)throw out_of_range("转为整数超出范围");
			return (int64_t)t;
		}
		void operator=(const long double&val){
			this->data=narrow(val,bits,sgn);
		}
};
class List{
	private:
		vector<Value> vals;
		vector<int> shape;
		string type;
		long long product(){
			long long size=1;
			for(auto i:shape){
				if(i<=0)throw out_of_range("数组大小不能为负或为零");
				size*=i;
				if(size>20000000LL)throw out_of_range("数组过大");
			}
			return size;
		}
		int flat_idx(const vector<long double>&idx){
			if(idx.size()!=shape.size())throw invalid_argument("下标维度数不匹配");
			int pos=0;
			for(int i=0;i<(int)shape.size();i++){
				if(!isfinite(idx[i]))throw invalid_argument("下标不应为NAN或无穷");
				else if(idx[i]>INT_MAX||idx[i]<INT_MIN)throw invalid_argument("下标应在int32范围内");
				else if(floorl(idx[i])!=idx[i])throw invalid_argument("下标应是整数");
				if(idx[i]<0||idx[i]>=shape[i])throw out_of_range("下标越界");
				pos=pos*shape[i]+idx[i];
			}
			return pos;
		}
	public:
		List(){}
		List(const vector<long double>&_shape,const string&_type){
			for(int i=0;i<(int)_shape.size();i++){
				if(!isfinite(_shape[i]))throw invalid_argument("大小不应为NAN或无穷");
				else if(_shape[i]>INT_MAX||_shape[i]<INT_MIN)throw invalid_argument("大小应在int32范围内");
				else if(floorl(_shape[i])!=_shape[i])throw invalid_argument("大小应是整数");
			}
			type=_type;
			shape=vector<int>(_shape.begin(),_shape.end());
			vals.resize(product(),Value(0,type));
		}
		Value& operator[](const vector<long double>&idx){
			return vals[flat_idx(idx)];
		}
		int getsize(int dim){
			if(dim<=0)throw out_of_range("维度不能为负");
			if(dim>(int)shape.size())throw out_of_range("维度超出原数组限制");
			return shape[dim-1];
		}
		int getdim(){
			return shape.size();
		}
};
class Scope{
	private:
		vector<unordered_map<string,Value>> vals={{}};
		vector<unordered_map<string,List>> lists={{}};
	public:
		void new_scope(){
			lists.push_back({});
			vals.push_back({});
		}
		void del_scope(){
			lists.pop_back();
			vals.pop_back();
		}
		bool has_val(const string&name){
			for(int i=vals.size()-1;i>=0;i--)if(vals[i].count(name))return 1;
			return 0;
		}
		bool has_list(const string&name){
			for(int i=lists.size()-1;i>=0;i--)if(lists[i].count(name))return 1;
			return 0;
		}
		bool has_val_in_this_scope(const string&name){
			if(vals.back().count(name))return 1;
			return 0;
		}
		bool has_list_in_this_scope(const string&name){
			if(lists.back().count(name))return 1;
			return 0;
		}
		Value& find_val(const string&name){
			for(int i=vals.size()-1;i>=0;i--)if(vals[i].count(name))return vals[i][name];
			throw invalid_argument("变量"+name+"未定义");
		}
		List& find_list(const string&name){
			for(int i=lists.size()-1;i>=0;i--)if(lists[i].count(name))return lists[i][name];
			throw invalid_argument("数组"+name+"未定义");
		}
		Value& new_val(const string&name){
			return vals.back()[name]=Value();
		}
		List& new_list(const string&name){
			return lists.back()[name]=List();
		}
		int size(){
			return vals.size();
		}
};
class CodeRunner{
	private:
		int line_num=INT_MIN;
		int offset;
		vector<pair<string,int>>line_nums;
		CodeBase base;
		Scope scope;
		vector<string> keyword_names={"var","output","input","let","list"
		,"putstr","new","size","turn","get","putchar","for","in","range"
		,"if","while","getchar","getch","func","call","include","else",
		"elif","ret","break","continue","finish"};
		bool is_valid_name(const string&name){
			if(name.empty())return 0;
			for(auto i:keyword_names)if(name==i)return 0;
			for(unsigned char i:name)if(i>127)return 0;
			if(isdigit(name[0]))return 0;
			for(auto i:name)if(!(isupper(i)||islower(i)||isdigit(i)||i=='_'))return 0;
			return 1;
		}
		void check_name(const string&name,int whith_name){
			string _name;
			if(whith_name==0)_name="变量";
			else if(whith_name==1)_name="数组";
			else if(whith_name==2)_name="函数";
			if(name=="_RET_")throw invalid_argument(_name+"名不能为_RET_");
			if(name.empty())throw invalid_argument(_name+"名为空");
			for(auto i:keyword_names)if(name==i)throw invalid_argument(_name+"名不能和关键字重复");
			for(unsigned char i:name)if(i>127)throw invalid_argument(_name+"名包含不合法字符");
			if(isdigit(name[0]))throw invalid_argument(_name+"名首字母不能是数字");
			for(auto i:name)if(!(isupper(i)||islower(i)||isdigit(i)||i=='_'))throw invalid_argument(_name+"名应为首字母不为数字的大写字母、小写字母、数字和下划线和组成");
		}
		int to_int(long double num){
			if(!isfinite(num))throw runtime_error("数值无限或为NAN");
			if(num<INT_MIN||num>INT_MAX)throw invalid_argument("数值超出int范围");
			return (int)trunc(num);
		}
		long double safe_stold(const string&str){
			size_t pos=0;
			long double res;
			try{
				res=stold(str,&pos);
			}catch(...){
				throw invalid_argument(str+":非法数字");
			}
			if(pos!=str.size())throw invalid_argument(str+":非法数字");
			return res;
		}
		long double calc_RPN(const string&token){
			string formula;
			if(token.size()>2&&token[0]=='`'&&token.back()=='`')formula=token.substr(1,token.size()-2);
			else if(token.size())formula=token;
			else throw invalid_argument("表达式解析失败");
			vector<string>tokens;
			string temp;
			for(int i=0;i<(int)formula.size();i++){
				switch(formula[i]){
					case ' ':
					case '\t':
						if(temp.empty())break;
						tokens.push_back(temp);
						temp="";
						break;
					default:
						temp+=formula[i];
				}
			}
			if(!temp.empty())tokens.push_back(temp);
			stack<long double>num;
			for(int i=0;i<(int)tokens.size();i++){
				if(isdigit((unsigned char)tokens[i][0])||(tokens[i][0]=='-'&&tokens[i].size()>1&&isdigit((unsigned char)tokens[i][1])))num.push(safe_stold(tokens[i]));
				else if(is_valid_name(tokens[i]))num.push(scope.find_val(tokens[i]).as_num());
				else if(tokens[i][0]=='-'&&tokens[i].size()>1&&is_valid_name(tokens[i].substr(1)))num.push(-scope.find_val(tokens[i].substr(1)).as_num());
				else if(tokens[i].size()==1&&(tokens[i][0]=='+'||tokens[i][0]=='-'||tokens[i][0]=='*'||tokens[i][0]=='/'||tokens[i][0]=='%')){
					if(num.size()>=2){
						char op=tokens[i][0];
						long double num2=num.top();num.pop();
						long double num1=num.top();num.pop();
						switch(op){
							case '+':num.push(num1+num2);break;
							case '-':num.push(num1-num2);break;
							case '*':num.push(num1*num2);break;
							case '/':
								if(num2==0)throw runtime_error("除零错误");
								num.push(num1/num2);
								break;
							case '%':
								if(num2==0)throw runtime_error("模零错误");
								if(num2!=truncl(num2)||num1!=truncl(num1))throw invalid_argument("浮点数不能取模");
								num.push(fmod(num1,num2));
								break;
							default:throw invalid_argument("表达式解析失败");
						}
					}else throw invalid_argument("表达式解析失败");
				}else throw invalid_argument("未知运算符");
			}
			if(num.size()!=1)throw invalid_argument("表达式格式错误");
			return num.top();
		}
		vector<long double> tokenizer(string token,bool is_idx){
			string name=(is_idx?"下标组":"for语句的range组");
			if(token.size()>2&&token[0]=='`'&&token.back()=='`')token=token.substr(1,token.size()-2);
			else if(!token.size())throw invalid_argument(name+"解析失败");
			if(token.empty())throw invalid_argument(name+"不能为空");
			string temp;
			vector<long double>nums;
			for(int i=0;i<(int)token.size();i++){
				switch(token[i]){
					case ',':{
						if(temp.empty())break;
						bool is_all_digit=1;
						for(int i=0;i<(int)temp.size();i++)if(!isdigit((unsigned char)temp[i]))is_all_digit=0;
						if(is_all_digit)nums.push_back(safe_stold(temp));
						else nums.push_back(calc_RPN(temp));
						temp="";
						break;
					}
					default:
						temp+=token[i];
				}
			}
			if(temp.size()){
				bool is_all_digit=1;
				for(int i=0;i<(int)temp.size();i++)if(!isdigit((unsigned char)temp[i]))is_all_digit=0;
				if(is_all_digit)nums.push_back(safe_stold(temp));
				else nums.push_back(calc_RPN(temp));
			}
			return nums;
		};
		bool cmp(const string&_num1,const string&op,const string&_num2){
			bool is_true=0;
			long double num1=calc_RPN(_num1);
			long double num2=calc_RPN(_num2);
			bool nm1=is_valid_name(_num1),nm2=is_valid_name(_num2);
			if(nm1&&!nm2){
				Value&v=scope.find_val(_num1);
				if(v.get_bits()<0)num2=v.narrow_like(num2);
			}else if(!nm1&&nm2){
				Value&v=scope.find_val(_num2);
				if(v.get_bits()<0)num1=v.narrow_like(num1);
			}
			if(op=="==")is_true=(num1==num2);
			else if(op=="!=")is_true=(num1!=num2);
			else if(op=="<")is_true=(num1<num2);
			else if(op==">")is_true=(num1>num2);
			else if(op==">=")is_true=(num1>=num2);
			else if(op=="<=")is_true=(num1<=num2);
			else throw invalid_argument("未知运算符:"+op);
			return is_true;
		};
		void run(){
			offset=base.offset;
			line_nums=base.line_nums;
			auto&funcs=base.funcs;
			vector<pair<int,int>>chain;
			stack<bool>loop_is_for;
			stack<int>func_call_scope_size;
			stack<int>loops_end_at;
			stack<int>loops_scope_cnt;
			stack<int>func_ret_stack;
			stack<int>func_loops_dep;
			stack<int>func_ends;
			for(line_num=0;line_num<(int)base.token.size();line_num++){
				auto&tokens=base.token[line_num];
				int pair_to=base.pair_to[line_num];
				if(tokens.empty())continue;
				string keyword=tokens[0];
				if(keyword=="var"){
					if(tokens.size()!=4)throw invalid_argument("传参数量不正确");
					check_name(tokens[1],0);
					if(scope.has_val_in_this_scope(tokens[1]))throw invalid_argument("变量\""+tokens[1]+"\"已定义");
					scope.new_val(tokens[1])=Value(calc_RPN(tokens[3]),tokens[2]);
				}else if(keyword=="output"){
					if(tokens.size()!=4)throw invalid_argument("传参数量不正确");
					bool is_all_digit=1;
					for(int i=0;i<(int)tokens[2].size();i++)if(!isdigit((unsigned char)tokens[2][i]))is_all_digit=0;
					if(tokens[2].size()<=2&&is_all_digit&&stoi(tokens[2])<100){
						if(tokens[3]=="ld")cout<<fixed<<setprecision(stoi(tokens[2]))<<calc_RPN(tokens[1]);
						else if(tokens[3]=="sc")cout<<scientific<<setprecision(stoi(tokens[2]))<<calc_RPN(tokens[1]);
						else throw invalid_argument("未知输出方式");
					}else throw invalid_argument("位数应为100内(不包括)正整数");
				}else if(keyword=="input"){
					if(tokens.size()!=2)throw invalid_argument("传参数量不正确");
					if(!scope.has_val(tokens[1]))throw invalid_argument("变量\""+tokens[1]+"\"未定义");
					long double temp;
					cin>>temp;
					if(cin.fail())throw runtime_error("输入错误");
					scope.find_val(tokens[1])=temp;
				}else if(keyword=="getch"){
					if(tokens.size()!=2)throw invalid_argument("传参数量不正确");
					if(!scope.has_val(tokens[1]))throw invalid_argument("变量\""+tokens[1]+"\"未定义");
					char temp;
					if(!cin.get(temp))throw runtime_error("输入错误");
					scope.find_val(tokens[1])=(long double)(unsigned char)temp;
				}else if(keyword=="getchar"){
					if(tokens.size()!=2)throw invalid_argument("传参数量不正确");
					if(!scope.has_val(tokens[1]))throw invalid_argument("变量\""+tokens[1]+"\"未定义");
					char temp;
					cin>>temp;
					if(cin.fail())throw runtime_error("输入错误");
					scope.find_val(tokens[1])=(long double)(unsigned char)temp;
				}else if(keyword=="let"){
					if(tokens.size()!=3)throw invalid_argument("传参数量不正确");
					scope.find_val(tokens[1])=calc_RPN(tokens[2]);
				}else if(keyword=="list"){
					if(tokens.size()!=5ull)throw invalid_argument("传参数量不正确");
					if(tokens[1]=="new"){
						check_name(tokens[2],1);
						if(scope.has_list_in_this_scope(tokens[2]))throw invalid_argument("数组\""+tokens[2]+"\"已定义");
						scope.new_list(tokens[2]);
						scope.find_list(tokens[2])=List(tokenizer(tokens[4],1),tokens[3]);
					}else if(tokens[1]=="size"){
						scope.find_val(tokens[4])=scope.find_list(tokens[2]).getsize(to_int(calc_RPN(tokens[3])));
					}else if(tokens[1]=="turn"){
						scope.find_list(tokens[2])[tokenizer(tokens[3],1)]=calc_RPN(tokens[4]);
					}else if(tokens[1]=="get"){
						scope.find_val(tokens[4])=scope.find_list(tokens[2])[tokenizer(tokens[3],1)].as_num();
					}else throw invalid_argument("未知list子命令:"+tokens[1]);
				}else if(keyword=="putstr"){
					if(tokens.size()!=2)throw invalid_argument("传参数量不正确");
					string final_token="";
					string str=tokens[1];
					if(str[0]=='`')str=str.substr(1,str.size()-2);
					for(int i=0;i<(int)str.size();i++){
						if(str[i]=='\\'){
							if(i==(int)str.size()-1)throw invalid_argument("转义字符未匹配");
							if(str[i+1]=='n')final_token+='\n';
							else if(str[i+1]=='\"')final_token+='\"';
							else if(str[i+1]=='\\')final_token+='\\';
							else if(str[i+1]=='t')final_token+='\t';
							else if(str[i+1]=='r')final_token+='\r';
							else if(str[i+1]=='s')final_token+=' ';
							else if(str[i+1]=='\'')final_token+='`';
							else throw invalid_argument("未用、弃用转义字符");
							i++;
						}else final_token+=str[i];
					}
					cout<<final_token;
				}else if(keyword=="putchar"){
					if(tokens.size()!=2)throw invalid_argument("传参数量不正确");
					long double val=calc_RPN(tokens[1]);
					if(!isfinite(val)||truncl(val)!=val)throw invalid_argument("值应是整数");
					if(val<-128||val>255)throw invalid_argument("值应在-128~255区间内(包括)");
					cout<<(char)((int)val);
				}else if(keyword=="for"){
					if(tokens.size()!=7)throw invalid_argument("传参数量不正确");
					check_name(tokens[2],0);
					scope.new_scope();
					scope.new_val(tokens[2]);
					if(tokens[3]=="in"&&tokens[4]=="range"&&tokens[6]==":"){
						scope.new_scope();
						if(tokens[5][0]=='`'&&tokens[5].back()=='`'){
							auto nums=tokenizer(tokens[5],0);
							if(nums.size()!=3)throw invalid_argument("for语法不正确");
							scope.find_val(tokens[2])=Value(nums[0],tokens[1]);
							Value&val=scope.find_val(tokens[2]);
							if(val.get_bits()>0&&val.narrow_like(nums[0])!=nums[0])throw invalid_argument("起始值超出变量范围");
							if(nums[2]==0)throw invalid_argument("步长不能为零");
							if(scope.find_val(tokens[2]).narrow_like(nums[2])==0)throw invalid_argument("步长在该类型下为零");
							if((nums[2]>0&&nums[0]>=nums[1])||(nums[2]<=0&&nums[0]<=nums[1]))line_num=pair_to,scope.del_scope(),scope.del_scope();
							else loops_end_at.push(pair_to),loops_scope_cnt.push(scope.size()-2),loop_is_for.push(1);
						}else{
							if(scope.find_list(tokens[5]).getdim()!=1)throw invalid_argument("列表\""+tokens[5]+"\"维度应为1");
							scope.find_val(tokens[2])=Value(0,tokens[1]);
							if(scope.find_list(tokens[5]).getsize(1)==0)line_num=pair_to,scope.del_scope(),scope.del_scope();
							else loops_end_at.push(pair_to),loops_scope_cnt.push(scope.size()-2),loop_is_for.push(1);
						}
					}else throw invalid_argument("for语法不正确");
				}else if(keyword=="func"){
					if(tokens.size()<5)throw invalid_argument("传参数量不正确");
					if(tokens.size()!=5+calc_RPN(tokens[3])*2)throw invalid_argument("传参数量不正确");
					check_name(tokens[1],2);
					line_num=pair_to;
				}else if(keyword=="call"){
					if(tokens.size()<3)throw invalid_argument("传参数量不正确");
					if(!funcs.count(tokens[1]))throw invalid_argument("函数不存在");
					auto func_tokens=base.token[funcs[tokens[1]]];
					if(func_tokens.size()<5)throw invalid_argument("函数行传参数量不正确");
					if(func_tokens.size()!=5+calc_RPN(func_tokens[3])*2)throw invalid_argument("函数行传参数量不正确");
					int size=0;
					if(tokens.size()==calc_RPN(tokens[2])+3)size=3;
					else if(tokens.size()==calc_RPN(tokens[2])+4)size=4;
					else throw invalid_argument("传参数量不正确");
					if(calc_RPN(tokens[2])!=calc_RPN(func_tokens[3]))throw invalid_argument("函数传参数量不正确");
					if(size==4&&func_tokens[2]=="void")throw invalid_argument("函数为void");
					if(size==3&&func_tokens[2]!="void")throw invalid_argument("函数不为void");
					int val_cnt=calc_RPN(func_tokens[3]);
					vector<long double> nums(val_cnt);
					for(int i=4;i<4+2*val_cnt;i+=2)nums[(i-4)/2]=calc_RPN(tokens[(i-4)/2+3]);
					func_call_scope_size.push(scope.size());
					scope.new_scope();
					for(int i=4;i<4+2*val_cnt;i+=2){
						check_name(func_tokens[i+1],0);
						if(scope.has_val_in_this_scope(func_tokens[i+1]))throw invalid_argument("变量\""+func_tokens[i+1]+"\"已定义");
						scope.new_val(func_tokens[i+1])=Value(nums[(i-4)/2],func_tokens[i]);
					}
					if(size==4&&!scope.has_val(tokens.back()))throw invalid_argument("变量\""+tokens.back()+"\"未定义");
					if(func_tokens[2]!="void")scope.new_val("_RET_")=Value(0,func_tokens[2]);
					else scope.new_val("_RET_");
					func_ret_stack.push(line_num);
					line_num=funcs[tokens[1]];
					func_loops_dep.push(loops_end_at.size());
					func_ends.push(base.pair_to[line_num]-1);
				}else if(keyword=="if"){
					if(tokens.size()!=5)throw invalid_argument("传参数量不正确");
					if(tokens[4]!=":")throw invalid_argument("if语法不正确");
					bool is_true=cmp(tokens[1],tokens[2],tokens[3]);
					if(!chain.empty()&&chain.back().second==scope.size())chain.back().first=is_true;
					else chain.push_back({is_true,scope.size()});
					if(!is_true){
						line_num=pair_to;
						continue;
					}
					scope.new_scope();
				}else if(keyword=="continue"){
					if(tokens.size()!=1)throw invalid_argument("传参数量不正确");
					if(loops_end_at.empty())throw invalid_argument("continue不在循环内");
					bool is_for=loop_is_for.top();
					while(scope.size()>loops_scope_cnt.top()+1+is_for)scope.del_scope();
					line_num=loops_end_at.top()-1;
				}else if(keyword=="break"){
					if(tokens.size()!=1)throw invalid_argument("传参数量不正确");
					if(loops_end_at.empty())throw invalid_argument("break不在循环内");
					line_num=loops_end_at.top();
					while(scope.size()>loops_scope_cnt.top())scope.del_scope();
					loop_is_for.pop();
					loops_end_at.pop();
					loops_scope_cnt.pop();
				}else if(keyword=="elif"){
					if(tokens.size()!=5)throw invalid_argument("传参数量不正确");
					if(tokens[4]!=":")throw invalid_argument("elif语法不正确");
					bool is_true=cmp(tokens[1],tokens[2],tokens[3]);
					bool has_chain=(!chain.empty()&&chain.back().second==scope.size());
					int last_if=has_chain?chain.back().first:-1;
					if((!is_true||last_if==1)&&has_chain){
						line_num=pair_to;
						continue;
					}else if(!has_chain)throw invalid_argument("elif找不到对应if");
					else chain.back().first=1;
					scope.new_scope();
				}else if(keyword=="else"){
					if(tokens.size()!=2)throw invalid_argument("传参数量不正确");
					if(tokens[1]!=":")throw invalid_argument("else语法不正确");
					bool has_chain=(!chain.empty()&&chain.back().second==scope.size());
					int last_if=has_chain?chain.back().first:-1;
					if(last_if==1&&has_chain){
						line_num=pair_to;
						continue;
					}else if(!has_chain)throw invalid_argument("else找不到对应if");
					scope.new_scope();
				}else if(keyword=="while"){
					if(tokens.size()!=5)throw invalid_argument("传参数量不正确");
					if(tokens[4]!=":")throw invalid_argument("while语法不正确");
					bool is_true=cmp(tokens[1],tokens[2],tokens[3]);
					if(!is_true){
						line_num=pair_to;
						continue;
					}else{
						loops_end_at.push(pair_to);
						loops_scope_cnt.push(scope.size());
						loop_is_for.push(0);
					}
					scope.new_scope();
				}else if(keyword==":"){
					auto tokens_pair_to=base.token[pair_to];
					if(tokens_pair_to[0]=="for"){
						scope.del_scope();
						scope.new_scope();
						auto cmp_range=[](long double val,long double end,long double step){
							if(step>0)return val<end;
							else return val>end;
						};
						if(tokens_pair_to[5][0]=='`'&&tokens_pair_to[5].back()=='`'){
							auto nums=tokenizer(tokens_pair_to[5],1);
							Value&val=scope.find_val(tokens_pair_to[2]);
							Value old_val=val;
							if(cmp_range(val.as_num()+nums[2],nums[1],nums[2]))val=val.as_num()+nums[2];
							long double new_val=old_val.as_num()+nums[2];
							if(!cmp_range(new_val,nums[1],nums[2])){
								scope.del_scope();
								scope.del_scope();
								loop_is_for.pop();
								loops_end_at.pop();
								loops_scope_cnt.pop();
							}else{
								val=new_val;
								if((nums[2]>0&&!(val.as_num()>old_val.as_num()))||(nums[2]<0&&!(val.as_num()<old_val.as_num())))throw invalid_argument("类型溢出,循环无法终止");
								line_num=pair_to;
							}
						}else{
							Value&val=scope.find_val(tokens_pair_to[2]);
							Value old_val=val;
							int _size=scope.find_list(tokens_pair_to[5]).getsize(1);
							if(val.as_num()<_size)val=val.as_num()+1;
							long double new_val=old_val.as_num()+1;
							if(new_val>=_size){
								scope.del_scope();
								scope.del_scope();
								loop_is_for.pop();
								loops_end_at.pop();
								loops_scope_cnt.pop();
							}else{
								val=new_val;
								if(val.as_num()<=old_val.as_num())throw invalid_argument("类型溢出,循环无法终止");
								line_num=pair_to;
							}
						}
					}else if(tokens_pair_to[0]=="if"){
						scope.del_scope();
					}else if(tokens_pair_to[0]=="elif"){
						scope.del_scope();
					}else if(tokens_pair_to[0]=="else"){
						scope.del_scope();
					}else if(tokens_pair_to[0]=="while"){
						bool is_true=cmp(tokens_pair_to[1],tokens_pair_to[2],tokens_pair_to[3]);
						if(is_true){
							line_num=pair_to;
							scope.del_scope();
							scope.new_scope();
						}else{
							scope.del_scope();
							loop_is_for.pop();
							loops_end_at.pop();
							loops_scope_cnt.pop();
						}
					}else if(tokens_pair_to[0]=="func"){
						if(func_ret_stack.empty())throw runtime_error("调用栈为空");
						line_num=func_ret_stack.top();
						auto call_tokens=base.token[line_num];
						Value ret_num;
						if(call_tokens.size()==calc_RPN(call_tokens[2])+4)ret_num=scope.find_val("_RET_");
						while(scope.size()>func_call_scope_size.top())scope.del_scope();
						while((int)loops_end_at.size()>func_loops_dep.top()){
							loops_end_at.pop();
							loops_scope_cnt.pop();
							loop_is_for.pop();
						}
						func_call_scope_size.pop();
						func_ret_stack.pop();
						func_loops_dep.pop();
						func_ends.pop();
						if(call_tokens.size()==calc_RPN(call_tokens[2])+4)scope.find_val(call_tokens.back())=ret_num.as_num();
					}
				}else if(keyword=="include"){
					continue;
				}else if(keyword=="ret"){
					if(tokens.size()!=1)throw invalid_argument("传参数量不正确");
					if(!func_ret_stack.size())throw invalid_argument("未位于函数内");
					line_num=func_ends.top();
				}else if(keyword=="finish"){
					if(tokens.size()>2)throw invalid_argument("传参数量不正确");
					if(tokens.size()==1)return;
					else if(tokens.size()==2){
						long double ret_num=calc_RPN(tokens[1]);
						if(!isfinite(ret_num))throw invalid_argument("退出码为Nan、inf或-inf");
						if(floor(ret_num)!=ret_num)throw invalid_argument("退出码应为整数");
						if(ret_num<0||ret_num>255)throw invalid_argument("退出码应在0~255之间");
						exit(ret_num);
					}
				}else throw invalid_argument("未知关键字:"+keyword);
				bool keep=0;
				if(keyword==":"&&pair_to>=0&&pair_to<(int)base.token.size()){
					const string&tokens_pair_to=base.token[pair_to][0];
					if(tokens_pair_to=="if"||tokens_pair_to=="elif")keep=1;
				}
				while(!chain.empty()&&scope.size()<chain.back().second)chain.pop_back();
				if(!chain.empty()&&!keep&&scope.size()==chain.back().second)chain.pop_back();
			}
		}
		void print_error(string error_type,string error_what){
			if(line_num!=INT_MIN){
				if(line_num+1-offset<=0){
					int sum=0;
					for(auto i:line_nums){
						sum+=i.second;
						if(sum>=line_num+1){
							cerr<<error_type+": 头文件:"<<i.first<<"第"<<to_string(line_num+1-sum+i.second)<<"行,"<<error_what<<'\n';
							break;
						}
					}
				}else cerr<<error_type+": 第"+to_string(line_num+1-offset)+"行,"<<error_what<<'\n';
			}else cerr<<error_what<<'\n';
			exit(1);
		}
		void run_code(const string&input_code){
			try{
				base.init(input_code);
				run();
			}catch(const invalid_argument&e){
				print_error("invalid argument",e.what());
				exit(1);
			}catch(const out_of_range&e){
				print_error("out of range",e.what());
				exit(1);
			}catch(const runtime_error&e){
				print_error("runtime error",e.what());
				exit(1);
			}catch(const domain_error&e){
				print_error("domain error",e.what());
				exit(1);
			}catch(const logic_error&e){
				print_error("logic error",e.what());
				exit(1);
			}catch(const exception&e){
				print_error("error",e.what());
				exit(1);
			}
		}
	public:
		CodeRunner(const string&input_code){
			run_code(input_code);
		}
};
int main(int argc,char*argv[]){
	vector<string> str_argv(argv,argv+argc);
	if(argc==3&&str_argv[1]=="-f"){
		ifstream file(str_argv[2]);
		if(!file){
			cout<<"无法打开文件:"+str_argv[2];
			return 1;
		}
		string code;
		try{
			code=string((istreambuf_iterator<char>(file)),istreambuf_iterator<char>());
		}catch(...){
			cerr<<"文件读取失败:"+str_argv[2];
			return 1;
		}
		if(code.size()>=3&&(unsigned char)code[0]==0xEF&&(unsigned char)code[1]==0xBB&&(unsigned char)code[2]==0xBF)code.erase(0,3);
		CodeRunner runner(code);
	}else{
		cout<<"使用-f打开文件";
		return 1;
	}
	return 0;
}
